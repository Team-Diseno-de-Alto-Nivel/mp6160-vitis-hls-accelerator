"""gem5 ARM64 system with the SystemC/TLM accelerator attached as an MMIO
peripheral, running the C driver from ../../program.

Topology:

    ARM64 TimingSimpleCPU (SE) — runs the driver ELF
        |
     membus --+-- SimpleMemory @0x80000000   <- image buffers, identity-mapped
              |
              +-- MemCtrl/DDR @0x100000000   <- process heap/stack
              |
              +-- Gem5ToTlmBridge32 @0x10000000 --> accel.tlm_target
              |
              +-- TlmToGem5Bridge32 <----------- accel.tlm_initiator

The SystemC Bus module is not used here: gem5's membus does the address decode.

Why SE mode rather than FS: the memory map below is fixed (control @0x10000000,
images @0x80000000/0x80600000). Under FS, 0x80000000 is Linux's own DRAM, so the
buffers would have to move and STRICT_DEVMEM would block /dev/mem on RAM anyway.
SE keeps the map exactly, and the driver's fopen/fread/fwrite still reach the
host filesystem. The cost is that /dev/mem does not exist, so the driver is
built with GEM5_SE=ON and dereferences the physical addresses this config
identity-maps into its page table.

Usage:
    gem5.opt configs/kv260_arm64.py --binary=../program/build/driver
"""

import argparse
import re
from pathlib import Path

import m5
from m5.objects import *

# Process heap/stack live here, deliberately away from the image buffers: gem5's
# SE page allocator hands out pages from the base of system.mem_ranges, so an
# identity map at 0x80000000 would collide with it. Keeping the image memory out
# of mem_ranges (but still a child of system, so it lands in system.memories and
# stays functionally accessible) is what keeps the two apart.
PROCESS_MEM_BASE = 0x100000000
PROCESS_MEM_SIZE = "512MiB"


def load_memory_map():
    """Parse ../../common/memory_map.h so this config cannot drift from the
    SystemC model and the driver, which include that same header."""
    header = Path(__file__).resolve().parent.parent.parent / "common" / "memory_map.h"
    pattern = re.compile(r"^#define\s+(\w+)\s+((?:0x)?[0-9A-Fa-f]+)U?L?L?\s*(?:/\*.*)?$")
    values = {}
    for line in header.read_text().splitlines():
        match = pattern.match(line.strip())
        if match:
            values[match.group(1)] = int(match.group(2), 0)
    for required in ("ACCEL_BASE", "ACCEL_REGION_SIZE", "GEM5_IMG_IN", "GEM5_IMG_OUT"):
        if required not in values:
            raise RuntimeError(f"{required} not found in {header}")
    return values


def build_system(binary_path: str, mmap: dict):
    system = System()
    system.clk_domain = SrcClockDomain(
        clock="250MHz", voltage_domain=VoltageDomain(voltage="1V")
    )
    system.mem_mode = "timing"
    system.mem_ranges = [AddrRange(PROCESS_MEM_BASE, size=PROCESS_MEM_SIZE)]

    system.membus = SystemXBar()

    system.cpu = ArmTimingSimpleCPU()
    # Disable Pointer Authentication: Ubuntu's static libc runs PAC (autia/autib)
    # in its startup, and when gem5 models PAC it authenticates for real, corrupts
    # the return address and derails. The KV260's Cortex-A53 (ARMv8.0) has no PAC,
    # so this matches real hardware. gem5 gates PAC on the ID register, not the SE
    # release list, so we zero the four PAuth fields of ID_AA64ISAR1_EL1 (default
    # 0x01011010 → 0x00011000, non-PAC fields preserved).
    system.cpu.isa = [ArmISA(id_aa64isar1_el1=0x00011000)]
    system.cpu.icache_port = system.membus.cpu_side_ports
    system.cpu.dcache_port = system.membus.cpu_side_ports
    system.cpu.createInterruptController()

    # DRAM backing the driver process itself.
    system.mem_ctrl = MemCtrl()
    system.mem_ctrl.dram = DDR3_1600_8x8(range=system.mem_ranges[0])
    system.mem_ctrl.port = system.membus.mem_side_ports

    # Memory backing the two image buffers. Sized to cover input (6,220,800 B at
    # GEM5_IMG_IN) and output (2,073,600 B at GEM5_IMG_OUT) contiguously.
    image_region_size = (mmap["GEM5_IMG_OUT"] - mmap["GEM5_IMG_IN"]) + 0x200000
    system.image_mem = SimpleMemory(
        range=AddrRange(mmap["GEM5_IMG_IN"], size=image_region_size)
    )
    system.image_mem.port = system.membus.mem_side_ports
    # Keep this buffer out of the SE physical-page allocator, which allocates from
    # the lowest reported memory (getConfAddrRanges pool 0). Reported, image_mem at
    # 0x80000000 sits below the process DRAM at 0x100000000, so the ELF's own pages
    # land on top of this identity-mapped region and the process overwrites its own
    # code. Not-reported keeps it fully addressable via the membus and identity map
    # but off-limits to the allocator. (The "known risk" in the module README.)
    system.image_mem.conf_table_reported = False

    # The SystemC accelerator, and its two bridges into gem5's memory system.
    system.accel = GrayscaleAccelerator()

    system.accel_regs = Gem5ToTlmBridge32(
        addr_ranges=[AddrRange(mmap["ACCEL_BASE"], size=mmap["ACCEL_REGION_SIZE"])]
    )
    system.accel_regs.gem5 = system.membus.mem_side_ports
    system.accel_regs.tlm = system.accel.tlm_target

    system.accel_dma = TlmToGem5Bridge32()
    system.accel_dma.gem5 = system.membus.cpu_side_ports
    system.accel_dma.tlm = system.accel.tlm_initiator

    system.system_port = system.membus.cpu_side_ports

    # Workload: the cross-compiled ARM64 driver.
    system.workload = SEWorkload.init_compatible(binary_path)
    process = Process()
    process.executable = binary_path
    process.cmd = [binary_path]
    system.cpu.workload = process
    system.cpu.createThreads()

    return system, process, image_region_size


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", required=True, help="cross-compiled ARM64 driver binary")
    args = parser.parse_args()

    binary_path = str(Path(args.binary).resolve())
    mmap = load_memory_map()

    system, process, image_region_size = build_system(binary_path, mmap)

    kernel = SystemC_Kernel(system=system)
    root = Root(full_system=False, systemc_kernel=kernel)

    m5.instantiate(None)

    # Identity-map the peripheral and the image buffers into the driver's page
    # table. This is what replaces mmap()/dev/mem: the driver dereferences these
    # physical addresses directly (see program/main.c, GEM5_SE path).
    process.map(mmap["ACCEL_BASE"], mmap["ACCEL_BASE"], mmap["ACCEL_REGION_SIZE"], False)
    process.map(mmap["GEM5_IMG_IN"], mmap["GEM5_IMG_IN"], image_region_size, True)

    print(f"gem5: running {binary_path}")
    exit_event = m5.simulate()
    print(f"gem5: exiting @ tick {m5.curTick()} because {exit_event.getCause()}")


if __name__ == "__m5_main__":
    main()

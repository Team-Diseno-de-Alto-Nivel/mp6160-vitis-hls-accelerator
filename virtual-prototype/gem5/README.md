# GEM5 + TLM 2.0 Bridge

Wires the SystemC/TLM accelerator model in
[`../systemc-model/src/accelerator`](../systemc-model/src/accelerator) into a
gem5 ARM64 system, so the CPU side of the flow is executed by gem5's simulated
core (running the C program in [`../program`](../program)) instead of the
`CPU` SystemC module used in the standalone `systemc-model` simulation.

The accelerator is not reimplemented or copied here — `src/SConscript` compiles
the very same `accelerator.cpp` the standalone sim builds, which is what keeps
the two tracks behaviourally identical.

## Layout

- `configs/kv260_arm64.py` — gem5 Python config: ARM64 SE-mode system, with the
  accelerator attached as an MMIO peripheral through gem5's SystemC/TLM bridges
  (`Gem5ToTlmBridge32` / `TlmToGem5Bridge32`, from `src/systemc/tlm_bridge/`).
  Parses `../common/memory_map.h` so it cannot drift from the model or driver.
- `src/` — the gem5 SimObject glue (`scons EXTRAS=` target):
  - `Accelerator.py` — `GrayscaleAccelerator`, a `SystemC_ScModule` exposing a
    TLM target socket (config registers) and a TLM initiator socket (DMA).
  - `accelerator_wrapper.{hh,cc}` — wraps the SystemC module's two sockets as
    gem5 ports via `sc_gem5::TlmTargetWrapper<32>` / `TlmInitiatorWrapper<32>`.
  - `SConscript` — declares the SimObject and the sources.

## Topology

```
ARM64 TimingSimpleCPU (SE) — runs the driver ELF
      |
   membus --+-- SimpleMemory @0x80000000   <- image buffers, identity-mapped
            |
            +-- MemCtrl/DDR @0x100000000   <- process heap/stack
            |
            +-- Gem5ToTlmBridge32 @0x10000000 --> accel.tlm_target
            |
            +-- TlmToGem5Bridge32 <----------- accel.tlm_initiator
```

The SystemC `Bus` module is not used on this path — gem5's membus does the
address decode, which is also why the standalone model's `0x03FFFFFF` RAM decode
limit is irrelevant here.

## Build & run

Requires a gem5 **source tree** (a `gem5.opt` on `$PATH` is not enough — the
accelerator is compiled into gem5):

```bash
make gem5 GEM5_ROOT=/path/to/gem5   # scons build/ARM/gem5.opt EXTRAS=<repo>/virtual-prototype
make run-vp GEM5_ROOT=/path/to/gem5 # cross-compiles the driver, then runs the config
make check                          # verifies the output against the BT.601 reference
```

`make run-vp` runs from the repo root on purpose: the driver opens
`images/input/image.raw` relative to its working directory, which gem5 SE
inherits.

## Why SE mode and not FS

The memory map is fixed (control `0x10000000`, images `0x80000000` /
`0x80600000`). Under full-system, `0x80000000` is Linux's own DRAM on gem5's ARM
platforms — the driver would scribble over the kernel, and `STRICT_DEVMEM` blocks
`/dev/mem` on RAM regions anyway. FS would therefore force the image buffers off
`0x80000000`, breaking the map. SE keeps the map exactly, boots in seconds, and
the driver's `fopen`/`fread`/`fwrite` still reach the host filesystem with no
rootfs or 9p.

The cost: SE has no `/dev/mem` (gem5's `mmap` emulation with a real fd copies
file contents into anonymous pages rather than mapping physical memory). So the
driver is built with `GEM5_SE=ON` and dereferences the physical addresses the
config identity-maps into its page table with `Process.map()`. Building the
driver with `-DGEM5_SE=OFF` restores the `/dev/mem` path for the real KV260.

## Known risk to validate on first run

Process heap/stack are placed at `0x100000000`, deliberately away from the image
buffers: gem5's SE page allocator hands out physical pages from the base of
`system.mem_ranges`, so an identity map at `0x80000000` would collide with it.
The image memory is kept out of `mem_ranges` but is still a child of `system`,
so it lands in `system.memories` (via the `Self.all` proxy) and stays
functionally accessible while the allocator never touches it. If a gem5 version
rejects a memory outside `mem_ranges`, the fallback is to advance the allocator
past the image region instead.

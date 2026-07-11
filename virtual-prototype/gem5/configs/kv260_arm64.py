"""gem5 config skeleton: ARM64 system with the SystemC/TLM accelerator
attached as an MMIO peripheral via gem5's SystemC co-simulation bridge.

Scaffolding only. TODO for the team:
  - Build gem5 with SystemC/TLM co-simulation support enabled.
  - Instantiate the ARM64 CPU, memory bus and DRAM.
  - Bind the Accelerator's TLM sockets (see ../../systemc-model/src/accelerator)
    to the gem5-side TLM<->gem5 bridge at the chosen MMIO base address.
  - Load the cross-compiled binary from ../../program as the workload.

Usage (once implemented): gem5.opt configs/kv260_arm64.py --binary=<path>
"""

import argparse

# import m5
# from m5.objects import *


def build_system(binary_path: str):
    # TODO: ArmSystem / SimpleMemory / cache hierarchy / CPU model
    # TODO: bridge accelerator TLM sockets into the gem5 memory map
    raise NotImplementedError("gem5 ARM64 system + TLM bridge not implemented yet")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", required=True, help="cross-compiled ARM64 driver binary")
    args = parser.parse_args()

    build_system(args.binary)

# GEM5 + TLM 2.0 Bridge

Wires the SystemC/TLM accelerator model in
[`../systemc-model/src/accelerator`](../systemc-model/src/accelerator) into a
gem5 ARM64 system, so the CPU side of the flow is executed by gem5's simulated
core (running the C program in [`../program`](../program)) instead of the
`CPU` SystemC module used in the standalone `systemc-model` simulation.

## Layout

- `configs/kv260_arm64.py` — gem5 Python config: ARM64 CPU + memory system,
  with the accelerator attached as an MMIO device via gem5's SystemC/TLM
  co-simulation bridge (`util/tlm` in the gem5 source tree —
  `Gem5ToTlmBridge` / `TlmToGem5Bridge`).

## Status

Scaffolding only — no bridge/CPU wiring implemented yet. TODO for the team:

1. Build gem5 with the SystemC co-simulation extension (`scons ... USE_SYSTEMC=1`,
   or the equivalent `util/tlm` build for the gem5 version in use).
2. Bind the `Accelerator` module's `target_socket` (config registers) and
   `init_socket` (RAM access) to the gem5-side bridge sockets.
3. The driver (`../program/src/main.c`) already fixes the memory map — match it
   exactly in the gem5 config (see `README.md`'s "GEM5 MMIO map"):
   accelerator control at `0x10000000`, input image DRAM at `0x80000000`,
   output image DRAM at `0x80600000`.
4. Point gem5 at the cross-compiled binary from `../program`.

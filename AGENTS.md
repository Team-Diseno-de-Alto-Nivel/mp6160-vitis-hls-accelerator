# AGENTS.md

Project instructions for all AI coding assistants working in this repository.

See @README.md for architecture overview, build instructions, diagrams, memory map, and transaction format.
See @docs/Enunciado.md for the full assignment specification (Spanish) — read it to understand what must be implemented and what deliverables are required.

This repo continues [mp6160-systemc-tlm-image-accelerator](https://github.com/Team-Diseno-de-Alto-Nivel/mp6160-systemc-tlm-image-accelerator) (previous evaluation): same accelerator behavior, now split into (1) an HLS implementation targeting real hardware and (2) a virtual prototype that drives the same SystemC accelerator model through gem5 + TLM 2.0 instead of a behavioral SystemC CPU.

## Non-obvious constraints

- **Two parallel accelerator implementations, same semantics**: `src/hls/` (Vitis HLS, synthesizable, targets the Kria KV260) and `src/model/accelerator` (behavioral SystemC/TLM, carried over from the previous evaluation). Keep both computing the same BT.601 grayscale conversion so results are comparable.
- **`src/common/memory_map.h` is the single source of truth for every address in the system**: the SystemC model and the C driver `#include` it, and the gem5 config parses it at startup. Never hardcode an address or register offset anywhere else — that is exactly how the accelerator, the driver and the docs previously ended up with three mutually incompatible register layouts.
- **`src/model` is a standalone regression sim**: it runs CPU+Bus+RAM+Disk+Accelerator with no gem5/HLS dependency, and its output is byte-identical to the previous evaluation's verified result. `make run-model && make check` is the fastest way to prove a change to the accelerator did not alter behaviour. Don't break it.
- **The gem5 path does not use the SystemC `Bus`**: gem5's membus does the address decode, and `gem5/src/SConscript` compiles the *same* `accelerator.cpp` the standalone sim builds. Never fork the accelerator into a gem5-specific copy.
- **The `src/hls/` kernel is implemented** as a 3-stage `#pragma HLS DATAFLOW` pipeline (`src/hls/grayscale_accel.cpp`): `read_rgb` (m_axi burst read) → `compute_gray` (BT.601, same float arithmetic as `src/model/utils/conversion.h`) → `write_gray` (m_axi burst write). It compiles and runs under plain host g++ too — `hls::stream` falls back to a std::queue shim via `__has_include`, so `make hls-host` validates it byte-for-byte against the golden without Vitis. Keep it bit-exact with the SystemC model.
- **Image format**: RAW binary, no header, 3 bytes/pixel RGB, row-major, 1920×1080. Total: 6,220,800 bytes input, 2,073,600 bytes output.
- **AI usage must be declared** in the README (prompts + type of use). Omitting it is treated as plagiarism per @docs/Enunciado.md.

## Coding conventions

- SystemC modules: one `.h` + `.cpp` pair per module under `src/<module>/`, `#pragma once`, C++17, `SC_THREAD` for processes.
- HLS kernel: interface pragmas (`m_axi`/`s_axilite`) live in the `.cpp`, not the header.

## Diagrams

All diagrams use Mermaid fenced blocks. GitHub renders them natively. Block diagram: `graph LR`. Sequence diagram: `sequenceDiagram`.

## AI Usage Declaration

When your AI assistant helps with this project, run `/log-ai` (available in Claude Code and GitHub Copilot Chat) to append a row to the `## AI-Assisted Development` table in `README.md`. Columns: **Model**, **Type of use**, **Prompt**. Required by the course — omitting it counts as plagiarism.

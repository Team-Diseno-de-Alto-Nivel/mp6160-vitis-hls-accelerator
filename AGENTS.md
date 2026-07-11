# AGENTS.md

Project instructions for all AI coding assistants working in this repository.

See @README.md for architecture overview, build instructions, diagrams, memory map, and transaction format.
See @docs/Enunciado.md for the full assignment specification (Spanish) — read it to understand what must be implemented and what deliverables are required.

This repo continues [mp6160-systemc-tlm-image-accelerator](https://github.com/Team-Diseno-de-Alto-Nivel/mp6160-systemc-tlm-image-accelerator) (previous evaluation): same accelerator behavior, now split into (1) an HLS implementation targeting real hardware and (2) a virtual prototype that drives the same SystemC accelerator model through gem5 + TLM 2.0 instead of a behavioral SystemC CPU.

## Non-obvious constraints

- **Two parallel accelerator implementations, same semantics**: `hls/` (Vitis HLS, synthesizable, targets the Kria KV260) and `virtual-prototype/systemc-model/src/accelerator` (behavioral SystemC/TLM, carried over from the previous evaluation). Keep both computing the same BT.601 grayscale conversion so results are comparable.
- **`virtual-prototype/systemc-model` is a standalone regression sim**: it still runs CPU+Bus+RAM+Disk+Accelerator exactly like the previous evaluation (`make run` there works with no gem5/HLS dependency). Don't break it while building the gem5 integration alongside it.
- **`virtual-prototype/gem5` + `virtual-prototype/program` are scaffolding, not yet implemented**: the ARM64 system config, the TLM<->gem5 bridge wiring, and the C driver's register access are all left as TODOs on purpose — do not fill them in unless explicitly asked to implement that functionality.
- **Image format**: RAW binary, no header, 3 bytes/pixel RGB, row-major, 1920×1080. Total: 6,220,800 bytes input, 2,073,600 bytes output.
- **AI usage must be declared** in the README (prompts + type of use). Omitting it is treated as plagiarism per @docs/Enunciado.md.

## Coding conventions

- SystemC modules: one `.h` + `.cpp` pair per module under `src/<module>/`, `#pragma once`, C++17, `SC_THREAD` for processes.
- HLS kernel: interface pragmas (`m_axi`/`s_axilite`) live in the `.cpp`, not the header.

## Diagrams

All diagrams use Mermaid fenced blocks. GitHub renders them natively. Block diagram: `graph LR`. Sequence diagram: `sequenceDiagram`.

## AI Usage Declaration

When your AI assistant helps with this project, run `/log-ai` (available in Claude Code and GitHub Copilot Chat) to append a row to the `## AI-Assisted Development` table in `README.md`. Columns: **Model**, **Type of use**, **Prompt**. Required by the course — omitting it counts as plagiarism.

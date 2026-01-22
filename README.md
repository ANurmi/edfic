# edfic

An interrupt controller for deadline-based interrupt handling.

## Requirements
- A suitable version of [Verilator](https://verilator.org/guide/latest/install.html) (Tested on 5.008)
- A waveform viewer, for example [Surfer](https://surfer-project.org/)

## Quickstart

Run the Verilator linter with `make lint`.

Compile and run the simulation with `make verilate simv`.

The simulation waveform is written to `$REPO/build/waveform.fst`.


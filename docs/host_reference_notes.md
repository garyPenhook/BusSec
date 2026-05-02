# Host Reference Notes

These notes track outside material that may help the BusSec host-side design.
They are references, not dependencies. BusSec should still grow its own Python
host package, CLI, protocol parser, tests, and small wrapper scripts.

## Direct Project Direction

- Use Python for the real host tool: serial transport, reconnect handling,
  binary framing, Intel HEX parsing, UPDI device profiles, replay, fuzz-case
  runners, and JSON/text reports.
- Use Bash only for thin wrappers, build/flash helpers, smoke tests, and calls
  into established external tools.
- Keep host protocol code separate from one-off lab scripts so future bus
  commands, replay, LOCKTEST, and reporting do not become shell glue.

## Local Glitcher Folder

The root `Glitcher/` folder is useful as reference material only.

- `Glitcher/output/pdf/fault-injection-library-professional.pdf` is a local
  PDF archive of Pico Glitcher/findus documentation. It may help with general
  concepts such as trigger timing, voltage-glitch terminology, parameter
  sweeps, ADC traces, and power-cycle control.
- `Glitcher/scripts/build_fault_injection_docs_pdf.py` is only a documentation
  PDF builder. It is not BusSec host code.
- The generated `Glitcher/tmp/` and `Glitcher/output/` assets are large
  generated artifacts. Do not treat them as source for BusSec.
- BusSec is not planned as a voltage-glitching platform, so this material
  should not drive the v1 firmware or host architecture.

## pd0wm Repositories

These repositories may be useful to study:

- `https://github.com/pd0wm/airtag-dump`
  - Useful pattern: a small host utility coordinates a serial helper board,
    sweeps timing parameters, attempts an operation through a separate debug
    adapter, and saves binary output.
  - BusSec mapping: replay, LOCKTEST orchestration, target power/reset
    sequencing, result capture, and host-side retry/report loops.
  - Do not reuse: AirTag/nRF52 target logic, protected-readout workflow, or
    hard-coded serial paths.

- `https://github.com/pd0wm/airtag-glitcher`
  - Useful pattern: a minimal USB CDC command protocol for a helper MCU with
    fixed-width command fields and explicit power/trigger actions.
  - BusSec mapping: simple command framing lessons for early host/firmware
    bring-up and smoke tests.
  - Do not reuse: STM32F103/Rust glitch firmware or glitch pulse logic.

- `https://github.com/pd0wm/panda`
  - Useful pattern: clear split between device firmware, Python userspace
    library, scripts, examples, and tests.
  - BusSec mapping: good model for `/host` as a real Python package with
    command modules, hardware-facing API, examples, and regression tests.
  - Do not reuse: CAN safety model, vehicle-specific logic, or panda USB API.

- `https://github.com/pd0wm/glasgow`
  - Useful pattern: Python-driven embedded interface tooling with a serious
    host/device boundary and many protocol applets.
  - BusSec mapping: architecture reference for a general embedded interface
    tool. Prefer studying upstream Glasgow documentation rather than depending
    on this fork.
  - Do not reuse: Glasgow applet framework unless BusSec explicitly changes
    scope.

- `https://github.com/pd0wm/ceedling-pic24-example`
  - Useful pattern: embedded C unit-test workflow around Microchip tooling.
  - BusSec mapping: possible reference if host-run C unit tests are added for
    protocol parsers or policy code.
  - Do not reuse: PIC24/XC16/MPLAB-X simulator project structure directly.

Low-priority references:

- `opendbc`, `socketcan-rs`, and `pandacan-rs` are CAN-focused. Keep them out
  of v1 because BusSec is not a CAN tool in the current scope.
- `svelte-hexedit` and `react-hexedit` may inspire a future binary-log viewer,
  but the current project is not a GUI application.

## Practical Reuse Targets

When the host tool starts, borrow the patterns, not the code:

- `host/bussec/transport.py`: serial open/reconnect/read/write and timeouts.
- `host/bussec/protocol.py`: text and binary frame encode/decode.
- `host/bussec/hexfile.py`: Intel HEX parsing and page planning.
- `host/bussec/profiles/`: UPDI device profiles and memory maps.
- `host/bussec/replay.py`: deterministic command replay.
- `host/bussec/locktest.py`: owned-target LOCKTEST workflow and report data.
- `host/tests/`: parser, profile, report, and fake-transport tests.

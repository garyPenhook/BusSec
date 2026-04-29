# BusSec-PIC32CM

BusSec-PIC32CM is a planned bench tool built around the Microchip
PIC32CM5164JH01048. When complete, it will act as a serial-bus front end for
embedded development, board bring-up, protocol debugging, parser testing, and
authorized security validation on operator-owned targets.

The firmware runs on the PIC32CM board. A host-side tool will connect over the
debugger CDC UART or a future dedicated serial link to configure buses, stream
logs, run transactions, and generate reports.

## Finished Tool Goals

The completed project will provide:

- UART monitoring, byte injection, send-hex commands, and pass-through bridge
  operation.
- I2C scanning, write, read, and write-read transactions with recovery support.
- SPI master transactions with manual chip-select control for devices such as
  SPI flash parts.
- UPDI bridge mode so host tools can communicate with AVR targets through the
  BusSec hardware.
- Native UPDI target discovery, signature reads, fuse/status inspection, flash
  programming, verification, and transcript logging for owned devices.
- Target reset, optional target power switching, voltage sensing, and trigger
  outputs for logic-analyzer correlation.
- Structured text and binary event logs with timestamps, overflow counters, and
  backpressure handling.
- Deterministic replay and fuzzing modes in later firmware versions.
- LOCKTEST mode for owner-authorized validation that target lock or security
  settings block documented readout paths.

## Intended Users

This project is intended for:

- Embedded firmware developers.
- Microcontroller board designers.
- Repair and recovery work on owned devices.
- Security researchers working only on owned or explicitly authorized targets.
- Lab and CTF users who need a programmable serial-bus tool.

## What The Firmware Will Do

The PIC32CM firmware will be a cooperative bare-metal application with no RTOS
in the first major release. Its job is to keep timing-sensitive and
hardware-facing work close to the MCU:

- Configure and drive SERCOM peripherals for UART, I2C, SPI, and UPDI.
- Use fixed ring buffers for bus traffic and host logs.
- Use interrupts and DMA where useful for high-volume RX/TX paths.
- Maintain timestamp counters for event logging.
- Enforce target-voltage checks before driving target-facing pins.
- Prevent UPDI half-duplex drive contention through explicit line-state
  control.
- Keep destructive or risky operations behind policy gates and explicit
  confirmation commands.

The firmware is not intended to be a high-speed logic analyzer or a storage
device. Long captures will stream to the host.

## Host Tooling

The host side is expected to provide:

- Command-line control of the BusSec firmware.
- Serial transport and reconnect handling.
- Text and binary log parsing.
- Intel HEX parsing and page streaming for programming workflows.
- Device profiles for supported UPDI targets.
- Replay tools and deterministic fuzz-case runners.
- LOCKTEST text and JSON report generation.

## UPDI Support

UPDI is a first-class bus family in this project. The planned progression is:

1. Normal UPDI electrical front end.
2. Raw byte transmit/receive with echo and collision monitoring.
3. Bridge mode for existing host tools.
4. Native attach and signature read.
5. Native erase, flash, verify, and fuse-read support for owned targets.
6. Fuse-write dry-run and controlled fuse-write workflows.
7. Optional high-voltage UPDI activation hardware on a separate board variant.

High-voltage UPDI activation is not part of the default hardware path. If
implemented, it must be physically isolated, current-limited, explicitly armed,
and tested first on dummy loads and sacrificial owned targets.

## LOCKTEST Mode

LOCKTEST is an owner-authorized validation mode. Its purpose is to verify that a
target configured as locked or protected cannot be read through documented
programming, debug, or bootloader interfaces.

Expected PASS behavior for a protected target is:

- Device identity and permitted status fields may be inspected.
- Protected firmware and protected data cannot be read.
- No erase, unlock, fuse write, high-voltage pulse, or programming operation
  occurs during the default LOCKTEST flow.

If a supposedly locked owned target allows protected readout, the tool should
record that as a target security failure.

## Non-Goals

The project will not try to become:

- A high-speed logic analyzer.
- A USB protocol analyzer.
- An Ethernet tool.
- A JTAG/SWD debugger.
- A voltage-glitching or power-analysis platform.
- A general-purpose protected-firmware extraction appliance.
- A GUI application.

## Completion Milestones

The planned feature path is:

1. Bring-up firmware with host CLI, `version`, `help`, `status`, and periodic
   status output.
2. UART A monitor and send-hex support.
3. I2C scan and basic I2C transactions.
4. SPI transfer support for known test devices.
5. Reset and trigger output control.
6. Binary logging and host parser support.
7. UPDI bridge mode.
8. Native UPDI attach, read-signature, fuse/status read, and programming.
9. LOCKTEST validation and report generation.
10. Optional high-voltage UPDI add-on support.
11. Deterministic fuzzing, replay, and later MITM/filter/scripted workflows.

## Current Repository Layout

```text
src/        Firmware source files
include/    Firmware headers
cmake/      MPLAB VS Code generated CMake project and user overlay
docs/       Local datasheets and project documentation
out/        Build artifacts
```

The detailed design plan is in
`BusSec_Toolkit_PIC32CM_UPDI_Design_Plan_v0_4_LOCKTEST.md`.

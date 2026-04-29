# BusSec Toolkit with UPDI Extension
## PIC32CM5164JH01048 UART / I²C / SPI / UPDI Security and Debug Toolkit

**Version:** 0.4 lock-validation design upgrade
**Date:** 2026-04-29
**Primary MCU:** Microchip PIC32CM5164JH01048
**Current bring-up board:** PIC32CM JH01 Curiosity Nano+ Touch Evaluation Kit
**Previous baseline:** STM32G071RB / NUCLEO-G071RB v0.2 design
**Role:** General-purpose serial-bus front-end for embedded development, debugging, parser testing, firmware bring-up, and authorized security research on operator-owned targets.
**Status:** Design phase with initial Curiosity Nano firmware bring-up started.

---

## 0. Hard scope statement

This design adds UPDI support to the existing BusSec UART/I²C/SPI toolkit.

The word **hacking** in this document means practical lab work on owned or explicitly authorized hardware: discovery, programming, debug-interface interaction, protocol tracing, parser robustness testing, and recovery of self-owned devices. The design intentionally supports strong diagnostic capability. It does **not** include unauthorized bypass of access controls, third-party target workflows, stealthy extraction, malware behavior, or invasive/fault-injection bypass automation in the default firmware. It **does** include owner-authorized lock-validation testing: controlled attempts to read protected memory through documented programming/debug/bootloader interfaces on targets explicitly marked as owned test targets, with denial normally treated as the expected PASS result.

The tool should be capable enough to be useful on a real bench:

- UART monitoring
- UART injection
- UART pass-through bridge
- I²C scanning
- I²C read/write/write-read transactions
- SPI master transactions
- Target reset control
- Structured event logging
- Reproducible bus fuzzing in later firmware versions
- UPDI serial bridge mode
- UPDI target discovery
- UPDI fuse/signature/status inspection
- UPDI flash programming for owned devices
- Optional UPDI high-voltage activation front-end for devices whose UPDI pin has been deliberately configured away from UPDI
- UPDI traffic logging and timing capture
- UPDI protocol research harness for owned targets

The PIC32CM5164JH01048 is a better fit than the STM32G071RB if the design is moved from a NUCLEO prototype to a custom board, mainly because it has 512 KB Flash and 64 KB SRAM instead of the G071RB's tighter 128 KB Flash / 36 KB SRAM budget. The CPU speed is lower than the G071RB plan, but this project is not DSP-bound.

---

## 1. Source facts used by this version

### 1.1 PIC32CM5164JH01048

Microchip lists the PIC32CM5164JH01048 as a 5 V Arm Cortex-M0+ based MCU with 512 KB Flash and 64 KB RAM. The PIC32CM JH data sheet also documents system features relevant to this project, including DMAC, event system, timers, SERCOM-style serial peripherals, and 48 MHz class operation.

Design consequences:

- Use `arm-none-eabi-gcc` with `-mcpu=cortex-m0plus -mthumb`.
- Use CMSIS-Core and the Microchip PIC32CM-JH DFP device pack for headers/startup/linker material.
- Treat Harmony/MCC output as reference material, not as mandatory architecture.
- Keep critical data paths simple: ring buffers, DMA where useful, direct-register or thin-driver peripheral code.

### 1.2 PIC32CM JH01 Curiosity Nano+ Touch bring-up board

The current firmware bring-up target is the PIC32CM JH01 Curiosity Nano+ Touch Evaluation Kit with the PIC32CM5164JH01048 mounted on the board. The on-board Nano debugger provides SWD programming/debugging and a CDC virtual COM port for host console work.

Board-specific consequences:

- Use the on-board debugger USB connection for initial programming, debugging, and host CLI.
- The host console/log channel uses the debugger CDC UART connection.
- On this board, debugger `CDC TX` connects to PIC32CM `PA00`, which is the target RX line.
- On this board, debugger `CDC RX` connects to PIC32CM `PA01`, which is the target TX line.
- Firmware maps the initial host console to `SERCOM1` on `PA00/PA01`.
- Treat these pins as board-bring-up assignments. A later custom BusSec board may move the host console to a different SERCOM/pin pair.
- PA00 and PA01 are shared with the Curiosity Nano edge connector, so external hardware on those pins can interfere with the host console.

### 1.3 UPDI

Microchip describes UPDI as a one-wire UART-based half-duplex interface for programming and debugging AVR devices. UPDI can be implemented as a dedicated pin on some devices, or as a shared pin that may be configured as GPIO or /RESET on smaller packages. When the shared UPDI function is disabled, high-voltage activation may be required to temporarily re-enable UPDI for that session.

Design consequences:

- UPDI must be treated as a single-wire, half-duplex UART-like electrical interface.
- The front-end must support target-voltage sensing and level compatibility.
- The firmware must prevent drive contention on the UPDI line.
- Optional high-voltage activation must be isolated, current-limited, explicitly armed, and treated as a separate hardware variant.

### 1.4 Existing v0.2 BusSec baseline

The existing v0.2 design scoped the tool as a streaming front-end, not a storage device or high-speed logic analyzer. It also correctly deferred fuzzing and MITM behavior until after the simple UART/I²C/SPI transport layers work. This v0.3 design keeps that discipline and adds UPDI as another bus family, not as a reason to explode scope.

---

## 2. Revised product definition

### 2.1 Product name

Working name:

```text
BusSec-PIC32CM
```

Long form:

```text
PIC32CM BusSec UART / I²C / SPI / UPDI Security and Debug Toolkit
```

### 2.2 Primary target users

- Embedded firmware developers
- Microcontroller board designers
- Security researchers working on owned/authorized targets
- CTF/lab users
- Repair and recovery users dealing with their own AVR boards
- People replacing a bag of USB-UART adapters, I²C scanners, SPI flash dongles, and UPDI programmers with one bench tool

### 2.3 Core philosophy

The MCU is a **streaming bus front-end**. It does not try to be a high-speed logic analyzer. It does not store long captures locally. It provides safe electrical interface hardware, timestamping, protocol transactions, reset/power control, structured logging, and deterministic test case generation.

---

## 3. Version plan

### 3.1 v0.1 alpha

Smallest useful target:

- Host CLI over serial
- `version`, `help`, `status`
- 1 Hz status frame
- UART A monitor at 115200 baud
- UART A sendhex
- I²C scan
- SPI xfer reading a W25Q-style JEDEC ID
- Reset A pulse
- Text logging
- Ring overflow counters

### 3.2 v1.0 dependable bus tool

Adds the core UART/I²C/SPI feature set:

- Host CLI
- Text and binary log formats
- UART monitor interrupt mode
- UART monitor DMA mode
- UART inject / sendhex
- UART bridge passthrough
- I²C scan
- I²C write/read/write-read
- I²C recovery
- SPI xfer with manual chip select
- Target reset pulse
- Trigger output
- Self-tests
- Host-side binary log parser

### 3.3 v1.1 UPDI programmer bridge

Adds practical UPDI support without overbuilding a full debugger:

- UPDI electrical front-end
- UPDI target-voltage sense
- UPDI serial bridge mode for host tools
- UPDI attach/detach
- UPDI idle/break handling
- UPDI target signature read through supported protocol routines
- UPDI fuse/status read where permitted
- UPDI chip erase and flash write for owned devices
- UPDI programming transcript logging
- UPDI line timing capture breadcrumbs

### 3.4 v1.2 native UPDI control

Adds native host CLI commands instead of relying only on bridge mode:

- `updi.ping`
- `updi.attach`
- `updi.read-sig`
- `updi.read-fuses`
- `updi.read-userrow`
- `updi.erase`
- `updi.flash.begin`
- `updi.flash.chunk`
- `updi.flash.commit`
- `updi.verify`
- `updi.reset-target`
- `updi.leave`

### 3.5 v1.3 optional high-voltage UPDI activation board

Adds optional hardware and firmware support for shared-UPDI devices:

- Separate HV front-end variant
- Target VDD monitoring
- Simple-pulse HV activation
- User-power-toggle HV activation
- Auto-power-toggle HV activation if target power switch is installed
- Safety interlocks
- Current-limited pulse output
- HV pulse logging
- Explicit confirmation commands

### 3.6 v1.4 owner-authorized LOCKTEST validation

Adds a dedicated lock/security validation mode. This mode is not a normal extraction feature. Its purpose is to confirm that a target configured as locked/protected cannot be read through documented programming, debug, or bootloader interfaces.

- `locktest.begin` with explicit owned-target confirmation
- Read device ID/signature
- Read lock/fuse/security status
- Attempt documented flash read and classify result
- Attempt documented EEPROM/user-row/config read where applicable
- Attempt bootloader read where applicable
- Attempt debugger attach/halt/read where applicable for the interface family
- Repeat after reset and power cycle
- Generate JSON and text reports
- Record whether any destructive action occurred; default must be none

Expected PASS result for a protected target:

```text
Device identity/status can be inspected where the silicon permits.
Protected firmware and protected data cannot be read.
No erase, unlock, fuse write, HV pulse, or programming operation occurred.
```

### 3.7 v2.0 fuzzing and replay

Adds controlled, reproducible fuzzing:

- UART fuzzer
- I²C fuzzer
- SPI fuzzer
- UPDI protocol robustness tester for owned lab targets only
- Deterministic PRNG seed logging
- Replay from seed and offset
- Target quiet/hang detection
- Optional target reset after hang

### 3.8 v3.0 MITM/filter/scripting

Adds more complex behavior:

- UART byte filter
- UART frame-aware filter/rewrite
- I²C scripted transactions
- SPI scripted transactions
- UPDI traffic annotation
- Host-side script runner

---

## 4. Non-goals

The design deliberately excludes these from the main branch:

- Not a high-speed logic analyzer
- Not a CAN tool in v1, despite the PIC32CM JH family having CAN-capable variants/boards
- Not a USB protocol analyzer
- Not an Ethernet tool
- Not a JTAG/SWD debugger
- Not a voltage-glitching rig
- Not a power-analysis rig
- Not a general-purpose protected-firmware extraction appliance. Owner-authorized LOCKTEST mode is permitted to attempt documented readout paths against owned targets to verify that lock bits/security settings actually block access.
- Not a GUI application
- Not a mass-storage capture device

UPDI is included because it is closely related to UART-like microcontroller programming/debugging and because it fits the serial-bus front-end model.

---

## 5. Hardware architecture

### 5.1 Top-level block diagram

```text
┌──────────────────────────────────────────────────────────────┐
│                       Host PC                                │
│                                                              │
│  bussec CLI                                                  │
│  log parser                                                  │
│  Intel HEX parser                                            │
│  UPDI bridge client / pymcuprog integration                  │
│  replay tools                                                │
└────────────────────────────┬─────────────────────────────────┘
                             │
                             │ USB-UART / debugger VCP
                             │ default 921600 8N1
                             │
┌────────────────────────────┴─────────────────────────────────┐
│              PIC32CM5164JH01048 BusSec board                 │
│                                                              │
│  Host serial console/log channel                             │
│  Target UART A monitor/inject/bridge                         │
│  Target UART B monitor/inject/bridge                         │
│  I²C master front-end                                        │
│  SPI master front-end                                        │
│  UPDI one-wire half-duplex front-end                         │
│  Optional UPDI HV activation module                          │
│  Timer/counter timestamp engine                              │
│  DMAC for high-volume RX/TX where appropriate                │
│  GPIO reset/power/trigger outputs                            │
└────────────────────────────┬─────────────────────────────────┘
                             │
                  ┌──────────┴──────────┐
                  │ Front-end hardware  │
                  │ level shifters,     │
                  │ current limits,     │
                  │ reset MOSFETs,      │
                  │ HV isolation,       │
                  │ pull-ups, headers   │
                  └──────────┬──────────┘
                             │
                  Target board(s) under test
```

### 5.2 PIC32CM operating voltage strategy

The PIC32CM5164JH01048 is a 5 V-capable Cortex-M0+ MCU, but the tool still needs proper front-end translation. Do not rely on bare GPIOs for unknown target boards.

Recommended strategy:

| Case | Front-end policy |
|---|---|
| 5 V target UART/SPI/UPDI | Use level shifter or same-domain operation only if board configuration guarantees safe VDD matching. |
| 3.3 V target UART/SPI/UPDI | Direct or translated operation is possible, but translated operation is still preferred for tool robustness. |
| 1.8 V target | Use translator. PIC32CM cannot simply run at 1.8 V. |
| I²C target | Use open-drain-compatible I²C translator and selectable pull-ups. |
| Unknown target voltage | Refuse to drive until VTGT is measured and accepted. |
| HV UPDI target | Use isolated HV path and disconnect normal logic translator during HV pulse. |

### 5.3 Power domains

Recommended board rails:

| Rail | Purpose |
|---|---|
| USB_5V | Input from host USB. |
| SYS_3V3 | PIC32CM core board rail if not running at 5 V. |
| TOOL_5V | Optional boosted or USB-derived tool rail. |
| VTGT_A | Target A sensed voltage. |
| VTGT_B | Target B sensed voltage. |
| VTGT_I2C | I²C target sensed voltage. |
| VTGT_SPI | SPI target sensed voltage. |
| VTGT_UPDI | UPDI target sensed voltage. |
| HV_UPDI | Optional isolated high-voltage activation rail. |

Every target-facing connector should have ground first, then voltage sense, then signals.

---

## 6. Front-end hardware

### 6.1 UART front-end

UART signals are directional. Do not use one generic auto-direction part for every UART signal.

Per UART channel:

```text
PIC32CM TX  -> level shifter -> target RX
PIC32CM RX  <- level shifter <- target TX
GND         <---------------> target GND
VTGT sense  <--------------- target VCC/reference
```

Recommended translator families:

- SN74LVC2T45 or similar for 3.3/5 V work
- SN74AXC2T45 class for lower-voltage work, but verify voltage range and 5 V tolerance before choosing
- TXU0102/TXU0204 class unidirectional UART translators where appropriate

Add series resistors near the tool side, typically 47–220 ohm for edge damping and fault current limiting.

### 6.2 I²C front-end

I²C is open-drain. Use parts intended for open-drain level shifting.

Recommended:

- TCA9306
- PCA9306
- Discrete BSS138 MOSFET shifter for low/medium speeds

Do not use TXB-series translators for I²C.

Include:

- Jumperable pull-ups to VTGT_I2C
- Pull-up values selectable for 100 kHz, 400 kHz, and short-cable lab use
- Bus stuck-low detection
- Optional SCL/SDA series resistors

### 6.3 SPI front-end

SPI is push-pull and speed-sensitive.

Recommended:

- SN74LVC4T245 / SN74LVC8T245 for 3.3/5 V
- SN74AXC4T245 class only if the complete voltage range fits the target plan

SPI directions:

```text
PIC32CM SCK  -> target
PIC32CM MOSI -> target
PIC32CM CSn  -> target
target MISO  -> PIC32CM
```

Avoid TXS/TXB auto-direction parts for SPI unless a specific prototype revision proves reliable on a scope.

### 6.4 Reset front-end

Use open-drain reset pulldown per target:

```text
PIC32CM GPIO -> 100 ohm -> small NMOS gate
NMOS source  -> GND
NMOS drain   -> target RESET
Target RESET -> target-side pull-up
```

This avoids directly driving reset high into an unknown voltage domain.

### 6.5 Trigger output

One or more trigger outputs should be available on a header:

- TRIG0: bus action breadcrumb
- TRIG1: error/backpressure marker
- TRIG2: fuzz/replay case boundary
- TRIG3: UPDI attach/HV/program event marker

A logic analyzer can then correlate tool actions with bus waveforms.

---

## 7. UPDI hardware design

### 7.1 UPDI connector

Minimum UPDI header:

```text
1 GND
2 VTGT_UPDI sense
3 UPDI_DATA
```

Recommended 6-pin bench header:

```text
1 GND
2 VTGT_UPDI sense
3 UPDI_DATA
4 TARGET_RESET_OD optional
5 TARGET_VCC_SWITCHED optional
6 TRIG_UPDI
```

Optional HV header, only on the HV-capable board variant:

```text
1 GND
2 VTGT_UPDI sense
3 UPDI_DATA_NORMAL
4 UPDI_HV_OUT
5 TARGET_VCC_SWITCHED
6 HV_ARM_STATUS
```

### 7.2 Simple SerialUPDI-compatible electrical mode

The simplest known-good UPDI electrical arrangement uses a UART TX path through a resistor into the UPDI node and a UART RX input sensing the same node.

Conceptual schematic:

```text
PIC32CM SERCOM_TX --[ 1k typical ]--+-- UPDI_DATA --> target UPDI
                                    |
PIC32CM SERCOM_RX ------------------+
                                    |
                                   ESD / clamp / translator as required
                                    |
                                   GND reference through target ground
```

Rules:

- TX must idle high when enabled.
- TX must be disabled or held inactive when receiving if the chosen hardware can contend.
- RX should always monitor the UPDI node for echo/collision diagnostics.
- The UPDI target voltage must be measured before enabling the driver.
- The firmware must support half-duplex turnaround timing.

This simple mode is best for a 3.3 V or 5 V target when the board is explicitly configured for that voltage.

### 7.3 Robust translated UPDI mode

For a product-grade tool, use a direction-controlled translator or analog switch arrangement instead of relying only on same-domain UART pins.

Preferred robust topology:

```text
              ┌─────────────────────────────┐
PIC32CM TX -> │ direction-controlled buffer  │ -> series R -> UPDI node
PIC32CM RX <- │ input buffer / sense path    │ <- protected sense node
DIR/EN    ->  │ controlled by firmware       │
              └─────────────────────────────┘

UPDI node -> ESD protection -> current limit -> target UPDI pin
VTGT_UPDI -> ADC divider/sense -> firmware voltage gate
```

Firmware states:

| State | TX buffer | RX sense | UPDI line behavior |
|---|---:|---:|---|
| DISCONNECTED | disabled | enabled if safe | Tool does not drive. |
| IDLE_LISTEN | disabled or idle-high safe | enabled | Wait for host/target state. |
| TX_ACTIVE | enabled | enabled | Transmit and monitor echo. |
| TURNAROUND | disabled | enabled | Wait for target response. |
| FAULT | disabled | enabled if safe | Report contention/voltage error. |

### 7.4 UPDI line protection

Minimum protection:

- Series resistor at tool output
- ESD diode appropriate for the selected voltage domain
- VTGT sense divider into ADC
- Firmware refuse-drive threshold
- Optional resettable fuse or current-limited target VCC switch

For HV-capable boards, normal protection is not enough. The HV path must be isolated so the 12 V-class pulse cannot backfeed the normal translator or the PIC32CM pin.

### 7.5 Optional UPDI high-voltage activation front-end

Some AVR parts have a shared UPDI pin that may be configured as GPIO or /RESET. Microchip documents high-voltage activation as a pulse used to re-enable UPDI functionality for the session; the pulse itself is an activation mechanism and does not program memory.

Hardware requirements:

- HV source generated locally or supplied externally
- HV source disabled by default
- HV source current-limited
- HV output switched by a device rated for the voltage
- Normal UPDI translator disconnected during HV pulse
- UPDI node voltage measured or at least sanity-checked
- Bleed resistor to discharge the UPDI node after pulse
- Physical solder jumper or hardware interlock to enable HV feature
- Firmware command interlock
- Host-side confirmation prompt

Suggested HV block:

```text
USB_5V / TOOL_5V
      |
      +--> boost converter / charge pump --> HV_UPDI reservoir
                                          |
                                          +--> current limit
                                          |
                                          +--> HV switch --> UPDI_HV_OUT --> UPDI node
                                                           |
                                                           +--> bleed resistor to safe rail/GND

PIC32CM controls:
  HV_ENABLE
  HV_SWITCH_GATE
  NORMAL_UPDI_ISOLATE
  TARGET_POWER_SWITCH
  VTGT_ADC
  HV_MON_ADC optional
```

HV firmware policy:

- HV is compiled out unless `ENABLE_UPDI_HV=1`.
- HV command is unavailable unless the board ID says HV hardware is installed.
- HV output is unavailable unless the hardware arm jumper is present.
- HV output is unavailable unless VTGT is in a valid range for the selected target family.
- HV output is unavailable unless the normal UPDI translator is isolated.
- HV action requires an explicit command token.
- HV action is logged with timestamp, target voltage, mode, and result.

Example command shape:

```text
updi.hv activate mode=simple-pulse target=tinyavr confirm=I_OWN_THIS_TARGET
updi.hv activate mode=user-power-toggle target=tinyavr confirm=I_OWN_THIS_TARGET
updi.hv activate mode=auto-power-toggle target=tinyavr confirm=I_OWN_THIS_TARGET
```

Do not apply HV activation to a board with unknown circuitry on the UPDI line. Microchip explicitly warns that circuitry on the UPDI line can interfere with the activation sequence or be damaged by the high-voltage pulse.

### 7.6 UPDI target power switching

Power switching is optional but useful.

Per UPDI target port:

- Load switch rated for expected current
- Current limit or fuse
- ADC sense after switch
- Optional discharge FET or bleed resistor
- `target.power on|off|cycle` command

Power-cycle support matters for safe HV activation when the shared UPDI pin may otherwise be actively driven by user firmware.

---

## 8. PIC32CM peripheral allocation

Exact pin mux must be verified against the PIC32CM5164JH01048 package, the selected board layout, and the DFP headers. The table below is an allocation plan, not a final schematic netlist. For the current PIC32CM JH01 Curiosity Nano+ Touch bring-up, the host console is fixed to the on-board debugger CDC wiring.

### 8.1 SERCOM allocation plan

| Function | Peripheral class | Notes |
|---|---|---|
| Host console/log | SERCOM1 USART | Curiosity Nano nEDBG CDC VCP: PA00 target RX from debugger CDC TX, PA01 target TX to debugger CDC RX. |
| UART A | SERCOM USART | Monitor/inject/bridge. |
| UART B | SERCOM USART | Optional second monitor/bridge. |
| UPDI | SERCOM USART | Half-duplex single-wire via external front-end. |
| I²C | SERCOM I²C master | Scanner/read/write/recovery. |
| SPI | SERCOM SPI master | Manual CS GPIOs. |

If the selected package/pinout does not permit all six cleanly, priorities are:

1. Host console
2. UART A
3. UPDI
4. I²C
5. SPI
6. UART B

### 8.2 Timer allocation

| Timer | Use |
|---|---|
| TC/TCC free-running counter | Timestamp low word. |
| Overflow IRQ | Timestamp high word extension. |
| One-shot timer | Reset pulse completion. |
| One-shot timer | UPDI turnaround timeout. |
| Periodic timer | 1 Hz status frame. |
| Optional timer | Fuzzer case scheduling. |

### 8.3 DMA allocation

DMAC channels should be allocated by active mode, not globally reserved.

| Use | DMA policy |
|---|---|
| Host binary log TX | Preferred. |
| UART monitor RX | Preferred for high baud. |
| UART bridge | Optional depending on latency. |
| SPI burst | TX/RX DMA for large bursts. |
| I²C | Polling or interrupt in v1; DMA optional later. |
| UPDI | Interrupt-driven first; DMA optional later. |

UPDI should start interrupt-driven. Its throughput is low enough that DMA adds complexity without much benefit.

---

## 9. Memory budget

The PIC32CM5164JH01048 has 64 KB SRAM. Use the extra RAM, but do not become careless.

### 9.1 Default v1.x static buffer allocation

| Buffer | Size |
|---|---:|
| Host RX command ring | 1024 bytes |
| Host TX/log ring | 8192 bytes |
| Event queue | 8192 bytes |
| UART A RX ring | 8192 bytes |
| UART B RX ring | 4096 bytes |
| UART bridge ring A->B | 2048 bytes |
| UART bridge ring B->A | 2048 bytes |
| SPI TX scratch | 2048 bytes |
| SPI RX scratch | 2048 bytes |
| I²C scratch | 512 bytes |
| UPDI RX/TX scratch | 1024 bytes |
| UPDI program page buffer | 1024 bytes |
| CLI input buffer | 512 bytes |
| CLI output scratch | 1024 bytes |
| Fuzzer queue, disabled in v1 | 2048 bytes |
| Misc config/state | ~4096 bytes |
| Stack reserve | 8192 bytes |

Target static use should stay below roughly 48 KB, leaving stack and margin.

### 9.2 Build rule

The build should fail if:

```text
.bss + .data + reserved_stack > 57344 bytes
```

A stricter warning threshold should fire at:

```text
.bss + .data + reserved_stack > 49152 bytes
```

### 9.3 No heap rule

Disable heap in the firmware unless a later feature proves it is necessary.

```text
malloc/free are banned from firmware runtime paths.
```

Host tools can use normal Python allocation.

---

## 10. Firmware architecture

### 10.1 Toolchain

Recommended:

```text
VS Code
arm-none-eabi-gcc
Microchip PIC32CM-JH DFP
CMSIS-Core
Make or CMake
pyOCD/J-Link/Microchip debugger path for flashing/debug
```

Compiler flags:

```make
CPUFLAGS = -mcpu=cortex-m0plus -mthumb
CFLAGS  += $(CPUFLAGS) -ffunction-sections -fdata-sections -Wall -Wextra
LDFLAGS += $(CPUFLAGS) -Wl,--gc-sections -T linker.ld
```

Build outputs:

```text
bussec.elf
bussec.hex
bussec.bin
bussec.map
bussec.size.txt
```

### 10.2 Firmware directory layout

```text
/firmware
  /src
    main.c
    board.c
    clock.c
    timestamp.c
    log.c
    cli.c
    ring.c
    dma_alloc.c
    uart_host.c
    uart_target.c
    i2c_tool.c
    spi_tool.c
    reset_tool.c
    power_switch.c
    trigger.c
    selftest.c
    vtgt_sense.c
    updi_phy.c
    updi_bridge.c
    updi_session.c
    updi_nvm.c
    updi_hv.c
    updi_log.c
    fuzz_uart.c
    fuzz_i2c.c
    fuzz_spi.c
    fuzz_updi.c
    prng.c
    isr.c
  /inc
    board.h
    config.h
    log.h
    cli.h
    ring.h
    bussec_protocol.h
    updi.h
    updi_priv.h
  /ld
    pic32cm5164jh01048.ld
  /startup
    startup_pic32cm5164jh01048.S
    system_pic32cm5164jh01048.c
  /test
    ring_tests.c
    log_tests.c
    parser_tests.c
  Makefile
  CMakeLists.txt
```

### 10.3 Host directory layout

```text
/host
  bussec/
    __init__.py
    cli.py
    serial_io.py
    command.py
    log_text.py
    log_binary.py
    ihex.py
    replay.py
    updi_bridge.py
    updi_flash.py
    device_db.py
    util.py
  tests/
    test_ihex.py
    test_binary_log.py
    test_replay.py
  pyproject.toml
```

Host dependencies:

- Python 3.11+
- pyserial
- optional: intelhex or internal Intel HEX parser
- optional: matplotlib for plots
- optional: pyyaml for scripted profiles

---

## 11. Concurrency model

Use cooperative bare-metal firmware. No RTOS in v1.

Priority model:

| Priority | Source | Job |
|---:|---|---|
| 0 | Hard fault / bus fault equivalent | Trap, log minimal state, halt or reset. |
| 1 | SERCOM error IRQ | Capture overrun/framing/parity/collision status. |
| 2 | DMA half/full IRQ | Move bulk data into rings. |
| 3 | Timestamp overflow IRQ | Extend timestamp high word. |
| 4 | Timer one-shot IRQ | Reset/power/UPDI timeout completion. |
| 5 | SERCOM RX/TX IRQ | Low-volume UART/UPDI/I²C/SPI stepping. |
| main | main loop | CLI parsing, command dispatch, formatting, housekeeping. |

Rules:

- ISRs do not allocate memory.
- ISRs do not print text.
- ISRs do not call the CLI parser.
- ISRs push compact events into fixed rings.
- Formatting happens in main loop.
- Long captures stream to the host.
- Any command that would block must become a state machine.

---

## 12. Logging system

### 12.1 Text mode

Human-readable. Good for low/medium throughput.

Example:

```text
1.234567890 UART A RX 0x55 'U' .
1.235001234 I2C 1 WR addr=0x68 w=[75] r=[68] ok
1.300000000 SPI 1 XFER cs=PA4 tx=[9F FF FF FF] rx=[00 EF 40 16] ok
1.410000000 UPDI ATTACH baud=225000 vtgt=4.98 ok
1.411000000 UPDI SIG bytes=[1E 96 0A] ok
1.500000000 RESET A mode=PULSE_LOW duration=10ms
1.600000000 OVERFLOW ring=uart_a_rx dropped=3
```

### 12.2 Binary mode

Binary logging is required for higher-rate captures.

Record header:

```c
#define LOG_MAGIC 0xE110E110u

struct log_record_header {
    uint32_t magic;
    uint64_t ts_ticks;
    uint16_t length;
    uint8_t  type;
    uint8_t  flags;
    uint8_t  payload[];
};
```

Record types:

```c
enum log_type {
    LOG_STATUS          = 0x01,
    LOG_ERROR           = 0x02,
    LOG_OVERFLOW        = 0x03,
    LOG_UART_BYTE       = 0x10,
    LOG_UART_BLOCK      = 0x11,
    LOG_I2C_TRANSACTION = 0x20,
    LOG_SPI_TRANSACTION = 0x30,
    LOG_RESET_EVENT     = 0x40,
    LOG_TRIGGER_EVENT   = 0x41,
    LOG_UPDI_BYTE       = 0x50,
    LOG_UPDI_FRAME      = 0x51,
    LOG_UPDI_SESSION    = 0x52,
    LOG_UPDI_NVM        = 0x53,
    LOG_UPDI_HV         = 0x54,
    LOG_FUZZ_CASE       = 0x60
};
```

### 12.3 Backpressure policy

If host TX/log ring exceeds 75% full:

1. Emit `LOG_BACKPRESSURE` if possible.
2. Preserve critical events: `ERROR`, `OVERFLOW`, `RESET`, `UPDI_HV`, `UPDI_ERASE`, `FUZZ_START`, `FUZZ_STOP`, `STATUS`.
3. Drop repeated low-priority byte logs first.
4. Increment per-type drop counters.
5. Report counters in the next `STATUS` frame.

Forwarding/programming operations should continue if possible even if detailed logging is reduced.

---

## 13. Command protocol

### 13.1 General command style

One command per line. Tokens are ASCII. Binary payloads are hex-encoded or transferred through a framed host protocol.

Examples:

```text
version
status
log.format text
log.format binary
uart.config A baud=115200 bits=8 parity=none stop=1
uart.monitor A mode=interrupt start
uart.sendhex A 55 aa 00 ff
uart.bridge A B mode=passthru start
i2c.config bus=1 speed=400000
i2c.scan bus=1
i2c.wr bus=1 addr=0x68 w=75 r=1
spi.config bus=1 mode=0 speed=1000000 cs=CS0
spi.xfer bus=1 cs=CS0 tx=9f,ff,ff,ff
reset A mode=pulse_low duration=10ms
updi.config baud=225000 vtgt=auto hv=off
updi.attach
updi.read-sig
```

### 13.2 Response conventions

| Prefix | Meaning |
|---|---|
| `>` | Echoed host command, optional. |
| timestamp | Event record. |
| `OK` | Command completed. |
| `!ERR` | Command failed. |
| `!WARN` | Non-fatal warning. |
| `#` | Comment/header. |

Error format:

```text
!ERR code=BAD_ARG field=baud value=99999999
!ERR code=VTGT_MISSING port=updi
!ERR code=UPDI_LOCKED action=read-flash
!ERR code=DMA_BUSY active=uart.monitor_dma requested=spi.burst
!ERR code=I2C_TIMEOUT detail="SCL held low" bus=1
```

---

## 14. UART feature design

### 14.1 UART monitor

Observe target UART and log bytes with timestamps and error flags.

Modes:

| Mode | Use |
|---|---|
| INTERRUPT | Low/medium baud, better per-byte timestamps. |
| DMA | Higher baud, lower CPU overhead, interpolated timestamps. |

Config:

```text
uart.config A baud=115200 bits=8 parity=none stop=1 invert_rx=false
uart.monitor A mode=interrupt start
uart.monitor A stop
```

### 14.2 UART inject

Transmit controlled bytes to the target.

```text
uart.drive A enable
uart.sendhex A 55 aa 00 ff
uart.inject A "help\r\n"
uart.break A duration=20ms
uart.drive A disable
```

Driving is explicit to avoid contention.

### 14.3 UART bridge

Raw pass-through first:

```text
uart.bridge A B mode=passthru start
uart.bridge A B stop
```

Bridge logs both original and forwarded bytes. Under backpressure, forwarding has priority over byte-by-byte logging.

---

## 15. I²C feature design

### 15.1 Scanner

```text
i2c.config bus=1 speed=100000
i2c.scan bus=1 start=0x08 end=0x77
```

Output:

```text
I2C SCAN start bus=1 speed=100000 range=0x08..0x77
0x3C ACK
0x50 ACK
0x68 ACK
SCAN done found=3
OK
```

### 15.2 Transactions

```text
i2c.w  bus=1 addr=0x68 bytes=6b,00
i2c.r  bus=1 addr=0x68 count=14
i2c.wr bus=1 addr=0x68 w=75 r=1
```

### 15.3 Bus recovery

```text
i2c.recover bus=1
```

Recovery sequence:

1. Disable I²C peripheral.
2. Configure SCL/SDA as GPIO open-drain.
3. Pulse SCL up to 9 times.
4. Generate STOP-like transition.
5. Re-enable I²C peripheral.
6. Report whether SDA released.

---

## 16. SPI feature design

### 16.1 Generic transaction

```text
spi.config bus=1 mode=0 speed=1000000 cs=CS0 cs_polarity=low
spi.xfer bus=1 cs=CS0 tx=9f,ff,ff,ff
```

Output:

```text
1.300000000 SPI 1 XFER cs=CS0 mode=0 speed=1000000 tx=[9F FF FF FF] rx=[00 EF 40 16] ok
```

### 16.2 v1.1 SPI flash helpers

```text
spi.flash_id bus=1 cs=CS0
spi.flash_read bus=1 cs=CS0 addr=0x000000 len=256
```

Keep the generic `spi.xfer` working first.

---

## 17. Reset and power control

### 17.1 Reset commands

```text
reset A mode=pulse_low duration=10ms
reset A mode=powercycle off=200ms stab=50ms
reset A mode=both off=200ms reset=10ms stab=50ms
```

### 17.2 Power commands

```text
power A on
power A off
power A cycle off=200ms stab=50ms
power status
```

Target power switching is optional hardware. Reset pulse is mandatory.

---

## 18. UPDI feature design

### 18.1 UPDI operating modes

| Mode | Version | Purpose |
|---|---|---|
| BRIDGE | v1.1 | Host tool such as pymcuprog talks through BusSec to target UPDI. |
| NATIVE_BASIC | v1.2 | BusSec host CLI performs attach, read signature, read fuses, erase, flash, verify. |
| MONITOR | v1.2 | Log UPDI bytes/frames for protocol study. |
| HV_ACTIVATE | v1.3 | Optional high-voltage activation on shared-UPDI devices. |
| LOCKTEST | v1.4 | Owner-authorized validation that target lock/security settings block documented readout paths. |
| RESEARCH | v2.0 | Reproducible timing/protocol tests on owned lab targets. |

### 18.2 UPDI configuration

```text
updi.config baud=225000 vtgt=auto parity=even stop=2 guard=auto
updi.config baud=115200 vtgt=5.0 parity=even stop=2 guard=auto
updi.status
```

The firmware should allow a small set of known-good baud rates first:

```text
57600
115200
225000
450000
```

Higher rates can be added after signal integrity is verified.

### 18.3 UPDI attach sequence as a firmware state machine

State machine:

```text
IDLE
  -> CHECK_VTGT
  -> CONFIG_PHY
  -> LINE_IDLE
  -> SEND_BREAK_OR_INIT
  -> SYNC_ATTEMPT
  -> READ_STATUS
  -> SESSION_ACTIVE
  -> ERROR or DETACHED
```

Pseudo-code:

```c
updi_result_t updi_attach(const updi_config_t *cfg)
{
    if (!vtgt_valid(PORT_UPDI)) return UPDI_ERR_VTGT;
    updi_phy_configure(cfg);
    updi_phy_idle_listen();

    for (unsigned attempt = 0; attempt < cfg->max_attempts; attempt++) {
        updi_phy_send_break();
        updi_delay_us(cfg->break_recovery_us);

        if (updi_link_sync() == UPDI_OK) {
            if (updi_read_status(&status) == UPDI_OK) {
                session.active = true;
                log_updi_session(UPDI_EVT_ATTACH_OK, status);
                return UPDI_OK;
            }
        }

        updi_phy_recover_line();
    }

    log_updi_session(UPDI_EVT_ATTACH_FAIL, 0);
    return UPDI_ERR_ATTACH;
}
```

The actual byte-level protocol constants should be taken from the relevant AVR data sheet, Microchip documentation, or a known-good open tool implementation during implementation. Do not invent magic constants in the design document.

### 18.4 UPDI bridge mode

Bridge mode is the fastest way to make the hardware useful.

Purpose:

```text
host pymcuprog/pyupdi/avrdude-style serial client
        <-> BusSec host serial
        <-> BusSec UPDI PHY
        <-> AVR target UPDI
```

Command:

```text
updi.bridge start baud=225000 escape=+++ vtgt=auto
```

Exit:

```text
+++            # guarded escape sequence, only accepted after idle gap
```

Bridge requirements:

- CLI parser is suspended during bridge mode.
- Host bytes are forwarded to UPDI TX according to half-duplex rules.
- Target bytes are forwarded back to host.
- Optional transcript logging can be enabled, but raw bridge timing must take priority.
- The bridge refuses to start if VTGT is missing or unsafe.

Status output before entering bridge:

```text
OK updi.bridge active baud=225000 vtgt=4.98 escape=+++ logging=off
```

### 18.5 Native UPDI commands

Basic commands:

```text
updi.attach
updi.detach
updi.ping
updi.read-sig
updi.read-fuses
updi.read-lock-status
updi.read-userrow
updi.reset-target
```

LOCKTEST commands:

```text
locktest.begin device=ATmega4809 target=owned confirm=I_OWN_THIS_TARGET
locktest.read-status
locktest.try-flash-read addr=0x0000 len=256
locktest.try-eeprom-read addr=0x0000 len=64
locktest.try-userrow-read
locktest.try-bootloader-read
locktest.reset-repeat count=3
locktest.powercycle-repeat count=3
locktest.report format=json
locktest.end
```

LOCKTEST does not require or expect successful extraction. For a locked target, the normal PASS result is `readout denied`. If a supposedly locked owned target allows readout, the tool records this as a target security failure.

Programming commands:

```text
updi.erase confirm=I_OWN_THIS_TARGET
updi.flash.begin device=ATmega4809 size=0x8000 crc32=0x12345678
updi.flash.chunk offset=0x0000 data=:HEXDATA...
updi.flash.commit
updi.verify
```

Host-side wrapper:

```bash
bussec updi flash --port /dev/ttyACM0 --device ATmega4809 --file firmware.hex --erase --verify
```

Firmware should not try to parse arbitrarily huge Intel HEX files in MCU RAM. The host parses HEX and streams page-sized chunks.

### 18.6 UPDI target database

Host-side database entry example:

```yaml
ATmega4809:
  signature: [0x1E, 0x96, 0x51]
  flash_size: 49152
  page_size: 128
  eeprom_size: 256
  userrow_size: 64
  nvm_controller: megaavr0
  updi_default_baud: 225000
  notes: "Verify exact values against data sheet."
```

Keep target-specific memory maps out of firmware unless needed for safety. Firmware should receive page size, address ranges, and operation type from the host after the host selects a device profile.

### 18.7 UPDI memory access policy

Memory operations are classified:

| Operation | Default | Notes |
|---|---|---|
| Read signature | Allowed | Device identification. |
| Read status | Allowed | Needed for session handling. |
| Read fuses | Allowed if target not locked and device profile permits. |
| Read user row | Allowed if target not locked and user confirms. |
| Read flash | Normal mode: unlocked owned targets only. LOCKTEST mode: documented read attempts allowed against owned locked targets for validation/reporting. |
| Chip erase | Allowed only after explicit confirmation; never part of default LOCKTEST. |
| Flash write | Allowed only after erase or page policy permits. |
| Lock/fuse write | Requires explicit confirmation and dry-run summary. |
| HV activation | Requires HV hardware, VTGT valid, and explicit confirmation. |

If a target reports a locked/protected state, normal mode must not perform flash readout. In LOCKTEST mode, after explicit owned-target confirmation, the tool may attempt documented readout paths and classify the result as allowed, denied, blocked, error, or inconclusive. Chip erase may be offered only outside default LOCKTEST because erase destroys protected contents before reprogramming.

### 18.8 UPDI fuse editing policy

Fuse writes can brick or disable access on small AVR parts.

Rules:

- Default command is read-only.
- Writes require `--confirm` and a pre-write diff.
- Host prints a warning if the change can disable UPDI or reset functionality.
- Firmware logs old value, new value, address, target signature, and confirmation token.
- Optional `fuse.safe-profile` command writes known-good lab profiles.

Example:

```text
updi.fuse.diff device=ATtiny817 addr=0x05 old=0xC5 new=0xC1 effect="UPDI pin function changes"
!WARN fuse change may alter future programming access
updi.fuse.write addr=0x05 value=0xC1 confirm=I_ACCEPT_FUSE_RISK
```

### 18.9 UPDI high-voltage activation commands

Status:

```text
updi.hv status
```

Example output:

```text
UPDI_HV installed=yes armed=no vtgt=5.01 hv_ready=no normal_isolated=yes
```

Arm:

```text
updi.hv arm confirm=HV_BOARD_CONNECTED
```

Activate:

```text
updi.hv activate mode=simple-pulse target=tinyavr confirm=I_OWN_THIS_TARGET
```

Auto power toggle:

```text
updi.hv activate mode=auto-power-toggle off=200ms stab=20ms target=tinyavr confirm=I_OWN_THIS_TARGET
```

Disarm:

```text
updi.hv disarm
```

Firmware state machine:

```text
HV_IDLE
  -> VERIFY_HW_PRESENT
  -> VERIFY_ARM_JUMPER
  -> VERIFY_VTGT
  -> ISOLATE_NORMAL_UPDI
  -> OPTIONALLY_POWER_CYCLE
  -> WAIT_FOR_SAFE_WINDOW
  -> FIRE_HV_PULSE
  -> DISCHARGE_HV_NODE
  -> RESTORE_NORMAL_UPDI
  -> SEND_REQUIRED_UPDI_KEY_OR_ATTACH_SEQUENCE
  -> VERIFY_SESSION
  -> HV_DONE / HV_ERROR
```

Logging:

```text
1.410000000 UPDI_HV mode=auto-power-toggle vtgt=5.02 result=pulse-fired
1.410065000 UPDI_HV key-window result=attach-ok
```

### 18.10 UPDI monitoring

Monitor mode captures traffic without necessarily controlling the target.

```text
updi.monitor start baud=225000 decode=basic
updi.monitor stop
```

Output levels:

| Decode level | Output |
|---|---|
| raw | Bytes with timestamps. |
| basic | Direction, byte, parity/error flags. |
| frame | Grouped operations where safely decoded. |
| annotate | Known commands annotated using target profile. |

The UPDI monitor should not claim nanosecond edge accuracy. It logs UART-level events, not analog signal-level details.

### 18.11 UPDI research mode

Research mode exists for owned lab targets.

Allowed research actions:

- Vary baud rate within known-safe ranges.
- Vary inter-byte delay.
- Vary attach retry timing.
- Log response timing.
- Send malformed but non-destructive commands to a sacrificial target.
- Replay a known programming session.
- Compare behavior across AVR families.

Disallowed in this firmware branch:

- Protected flash extraction attempts.
- Fault injection to bypass lock/protection.
- Glitch scheduling coupled to security-state transitions.
- Hidden readout paths that ignore lock status.

Research command examples:

```text
updi.research timing-sweep baud=115200..450000 target=ATmega4809 duration=10s
updi.research attach-retry count=100 delay=1ms..20ms
updi.research malformed-safe profile=sync-only seed=0x12345678 cases=1000
```

The `malformed-safe` profile must be constrained to session/sync/status behavior unless the target is explicitly marked sacrificial.

---

## 19. UPDI implementation detail

### 19.1 `updi_phy.c`

Responsibilities:

- Configure selected SERCOM as USART.
- Configure parity/stop bits for UPDI.
- Control TX enable / translator direction.
- Send break/idle sequence.
- Send bytes.
- Receive bytes with timeout.
- Monitor echo/collision.
- Detect line stuck low/high.
- Report framing/parity/overrun errors.

API sketch:

```c
typedef struct {
    uint32_t baud;
    uint8_t parity;
    uint8_t stop_bits;
    uint16_t turnaround_us;
    uint16_t rx_timeout_us;
} updi_phy_config_t;

typedef enum {
    UPDI_PHY_OK = 0,
    UPDI_PHY_ERR_VTGT,
    UPDI_PHY_ERR_TIMEOUT,
    UPDI_PHY_ERR_FRAMING,
    UPDI_PHY_ERR_PARITY,
    UPDI_PHY_ERR_COLLISION,
    UPDI_PHY_ERR_STUCK_LOW,
    UPDI_PHY_ERR_STUCK_HIGH
} updi_phy_result_t;

updi_phy_result_t updi_phy_init(const updi_phy_config_t *cfg);
updi_phy_result_t updi_phy_send_break(void);
updi_phy_result_t updi_phy_write(const uint8_t *buf, size_t len);
updi_phy_result_t updi_phy_read(uint8_t *buf, size_t len, uint32_t timeout_us);
void updi_phy_disconnect(void);
```

### 19.2 `updi_link.c`

Responsibilities:

- UPDI sync and link-layer operations.
- Retry policy.
- Acknowledgment/error interpretation.
- Resynchronization after bad byte.
- Frame logging.

API sketch:

```c
typedef struct {
    uint32_t retries;
    uint32_t sync_count;
    uint32_t timeout_count;
    uint32_t parity_error_count;
    uint32_t collision_count;
} updi_link_stats_t;

updi_result_t updi_link_sync(void);
updi_result_t updi_link_read_status(uint8_t *status);
updi_result_t updi_link_send_key(const uint8_t *key, size_t len);
updi_result_t updi_link_reset(void);
```

### 19.3 `updi_session.c`

Responsibilities:

- Attach/detach lifecycle.
- Target identification.
- Session state.
- Lock/protection-state tracking.
- Safety gating.

Session struct:

```c
typedef struct {
    bool active;
    bool locked;
    bool hv_used;
    uint32_t baud;
    uint32_t vtgt_mv;
    uint8_t signature[3];
    uint32_t attach_count;
    uint32_t error_count;
} updi_session_t;
```

### 19.4 `updi_nvm.c`

Responsibilities:

- Device-profile-based memory programming.
- Page buffering.
- Erase/write/verify sequences.
- Range checks.
- Lock-state enforcement.
- Logging destructive actions.

Rules:

- No address outside declared device profile.
- No flash read if lock/protection state says no.
- No chip erase without explicit confirmation.
- No fuse write without preflight diff.
- All erase/write operations emit critical log records.

### 19.5 `updi_hv.c`

Responsibilities:

- Detect HV-capable board variant.
- Detect arm jumper.
- Control boost converter/charge pump.
- Isolate normal UPDI path.
- Control HV pulse switch.
- Manage target power toggle if available.
- Discharge HV node.
- Log result.

API sketch:

```c
typedef enum {
    UPDI_HV_SIMPLE_PULSE,
    UPDI_HV_USER_POWER_TOGGLE,
    UPDI_HV_AUTO_POWER_TOGGLE
} updi_hv_mode_t;

typedef struct {
    updi_hv_mode_t mode;
    uint32_t off_ms;
    uint32_t stabilization_ms;
    bool require_confirm;
} updi_hv_request_t;

updi_result_t updi_hv_status(updi_hv_status_t *out);
updi_result_t updi_hv_activate(const updi_hv_request_t *req);
void updi_hv_emergency_off(void);
```

Emergency-off conditions:

- Unexpected VTGT drop
- HV rail over/under expected range
- Normal UPDI isolation failed
- Arm jumper removed
- Firmware watchdog nearing timeout
- Host abort command received before pulse

---

## 20. Security and safety policy in firmware

### 20.1 Command classification

| Class | Examples | Confirmation required |
|---|---|---|
| Read-only diagnostic | status, read signature, scan, monitor | No |
| Bus-driving non-destructive | UART inject, I²C read, SPI JEDEC ID | Sometimes |
| Destructive target operation | chip erase, flash write, fuse write | Yes |
| Electrical-risk operation | HV UPDI, target power switch | Yes |
| Research/fuzzing | malformed protocol tests | Yes, plus rate limits |

### 20.2 Confirmation tokens

Use explicit text tokens instead of single-letter confirmations.

Examples:

```text
confirm=I_OWN_THIS_TARGET
confirm=I_ACCEPT_FUSE_RISK
confirm=HV_BOARD_CONNECTED
confirm=SACRIFICIAL_TARGET
```

### 20.3 Lock/protection behavior

If the target reports locked/protected state:

- Signature/status read remains allowed where the silicon permits.
- In NORMAL mode, flash read is refused.
- In LOCKTEST mode, documented readout attempts are allowed after owned-target confirmation; the result is recorded as `denied`, `allowed`, `blocked`, `tool-refused`, `transport-error`, or `inconclusive`.
- EEPROM/user-row/config reads follow the same NORMAL vs LOCKTEST distinction, with device-profile limits.
- Chip erase is allowed only outside default LOCKTEST and only with confirmation, because erase destroys contents before programming.
- Attempts are logged.

Error:

```text
!ERR code=UPDI_LOCKED action=read-flash policy=normal-mode-refuse hint=use-locktest-for-owned-validation
```

### 20.4 Audit log

Destructive or risky actions emit persistent host-visible records:

```text
UPDI_AUDIT action=erase device=ATmega4809 sig=1E9651 confirm=I_OWN_THIS_TARGET
UPDI_AUDIT action=fuse_write addr=0x05 old=0xC5 new=0xC1 confirm=I_ACCEPT_FUSE_RISK
UPDI_AUDIT action=hv_activate mode=simple vtgt=5.02 confirm=I_OWN_THIS_TARGET
```

### 20.5 Operating modes and policy gates

The firmware and host tools must distinguish four operating modes:

| Mode | Purpose | Protected read attempts | Destructive actions |
|---|---|---:|---:|
| NORMAL | Bus monitoring, diagnostics, basic UPDI status work. | No | No |
| PROGRAM | Programming owned targets. | No protected readout. | Confirmation required. |
| LOCKTEST | Owner-authorized validation of target protection. | Yes, through documented interfaces only. | No by default. |
| RESEARCH_DEV | Private developer/lab build. | Build-config dependent. | Build-config and confirmation required. |

All security-sensitive operations must pass through one policy gate. Confirmation tokens do not override protected readout rules in NORMAL or PROGRAM mode. LOCKTEST is the only normal build mode allowed to attempt protected-memory reads, and only for reporting whether protection holds.

Suggested policy API:

```c
typedef enum {
    BUSSEC_MODE_NORMAL,
    BUSSEC_MODE_PROGRAM,
    BUSSEC_MODE_LOCKTEST,
    BUSSEC_MODE_RESEARCH_DEV
} bussec_mode_t;

typedef enum {
    BUSSEC_OP_READ_SIG,
    BUSSEC_OP_READ_STATUS,
    BUSSEC_OP_READ_FUSES,
    BUSSEC_OP_READ_FLASH,
    BUSSEC_OP_READ_EEPROM,
    BUSSEC_OP_READ_USERROW,
    BUSSEC_OP_CHIP_ERASE,
    BUSSEC_OP_FLASH_WRITE,
    BUSSEC_OP_FUSE_WRITE,
    BUSSEC_OP_HV_ACTIVATE,
    BUSSEC_OP_FUZZ
} bussec_op_t;

typedef enum {
    BUSSEC_POLICY_ALLOW,
    BUSSEC_POLICY_DENY_LOCKED_NORMAL_MODE,
    BUSSEC_POLICY_DENY_CONFIRM_REQUIRED,
    BUSSEC_POLICY_DENY_VTGT,
    BUSSEC_POLICY_DENY_PROFILE,
    BUSSEC_POLICY_DENY_BUILD_CONFIG,
    BUSSEC_POLICY_DENY_HV_NOT_ARMED
} bussec_policy_result_t;
```

### 20.6 LOCKTEST result classification

LOCKTEST reports the result of each attempted read; it does not assume success or failure.

| Result | Meaning |
|---|---|
| `denied-by-target` | Target/debug interface rejected the read. Expected PASS for a locked target. |
| `denied-by-tool-profile` | Device profile says the attempted read is outside the allowed validation scope. |
| `allowed-read-succeeded` | Read succeeded. PASS for unlocked target; FAIL for a supposedly locked target. |
| `all-ff-or-all-00` | Data pattern is suspicious; classify as inconclusive unless device docs define behavior. |
| `transport-error` | Electrical/protocol/tool connection failed. Inconclusive. |
| `timeout` | Target did not respond. Inconclusive unless repeatable and documented. |
| `not-supported` | Interface or target family does not support that test. |

LOCKTEST reports must include:

```text
Target device/profile
Detected signature/device ID
Detected lock/security state
Tool firmware version/build mode
Interface used
Each attempted read path
Result classification
Whether any destructive action occurred
Final PASS/FAIL/INCONCLUSIVE verdict
```

---

## 21. Host software design

### 21.1 CLI examples

General:

```bash
bussec --port /dev/ttyACM0 status
bussec --port /dev/ttyACM0 monitor uart A --baud 115200
bussec --port /dev/ttyACM0 i2c scan --speed 100000
bussec --port /dev/ttyACM0 spi xfer --mode 0 --speed 1000000 --cs CS0 --tx 9f,ff,ff,ff
```

UPDI:

```bash
bussec --port /dev/ttyACM0 updi attach --baud 225000
bussec --port /dev/ttyACM0 updi read-sig
bussec --port /dev/ttyACM0 updi read-fuses --device ATmega4809
bussec --port /dev/ttyACM0 updi flash --device ATmega4809 --file app.hex --erase --verify
bussec --port /dev/ttyACM0 updi bridge --baud 225000
bussec --port /dev/ttyACM0 updi hv-activate --mode simple-pulse --confirm I_OWN_THIS_TARGET
```

LOCKTEST:

```bash
bussec --port /dev/ttyACM0 locktest begin --device ATmega4809 --confirm I_OWN_THIS_TARGET
bussec --port /dev/ttyACM0 locktest read-status
bussec --port /dev/ttyACM0 locktest try-flash-read --addr 0x0000 --len 256
bussec --port /dev/ttyACM0 locktest try-eeprom-read --addr 0x0000 --len 64
bussec --port /dev/ttyACM0 locktest powercycle-repeat --count 3
bussec --port /dev/ttyACM0 locktest report --format json --out locktest_report.json
bussec --port /dev/ttyACM0 locktest end
```

### 21.2 Host-side Intel HEX flow

```text
load .hex
  -> parse records
  -> validate address ranges against device profile
  -> compute CRC32
  -> ask firmware for target signature
  -> compare signature to selected device
  -> issue erase if requested
  -> stream page chunks
  -> verify by page CRC or readback if permitted
  -> reset target
```

### 21.3 Host-side UPDI bridge integration

Two options:

1. Native BusSec CLI wrapper handles flashing itself.
2. BusSec enters bridge mode and the user runs a known external programmer tool against a virtual or real serial endpoint.

Simpler v1.1 implementation:

```text
Use BusSec as a physical UPDI serial bridge.
Use pymcuprog or pyupdi on the host for actual programming.
```

Better v1.2 implementation:

```text
Host BusSec Python CLI handles device profiles and page streaming.
Firmware handles low-level UPDI transactions.
```

---

## 22. Fuzzing design

### 22.1 General fuzzer policy

Fuzzing is deferred until v2.0. All fuzzers are deterministic and rate-limited.

Every fuzz run logs:

```text
strategy
seed
case counter
bus/channel
target profile
rate
max length
reset policy
```

### 22.2 PRNG

Use xorshift32 initially:

```c
uint32_t xorshift32(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}
```

Good enough for reproducibility. Not cryptographic. Do not describe it as cryptographic.

### 22.3 UART fuzzer

Strategies:

- RANDOM_UNIFORM
- RANDOM_BIASED
- ASCII_PRINTABLE
- DICTIONARY
- MUTATION
- FRAMING_ERROR

### 22.4 I²C fuzzer

Strategies:

- REG_RANDOM
- LEN_BOUNDARY
- READ_OVERRUN
- MID_TRANSACTION_ABORT
- MUTATION

### 22.5 SPI fuzzer

Strategies:

- OPCODE_SWEEP
- RANDOM_BYTES
- LENGTH_FUZZ
- SPEED_SWEEP
- MUTATION

### 22.6 UPDI research/fuzz harness

UPDI fuzzing must be constrained because destructive commands exist.

Profiles:

| Profile | Allowed behavior |
|---|---|
| sync-only | Break/sync/status timing only. |
| attach-only | Attach/detach timing only. |
| read-only | Signature/status/fuse reads only on unlocked owned target. |
| sacrificial | Malformed command tests on a target marked sacrificial. |
| destructive | Requires separate compile-time flag and explicit confirmation. |

Default profile is `sync-only`.

Commands:

```text
updi.fuzz profile=sync-only seed=0x12345678 cases=1000 rate=10/s
updi.fuzz profile=attach-only seed=0x12345678 cases=1000 rate=5/s
updi.fuzz profile=sacrificial seed=0x12345678 cases=100 confirm=SACRIFICIAL_TARGET
```

UPDI fuzzing does not run if:

- VTGT is invalid.
- Target profile is missing.
- HV is armed.
- A programming operation is active.
- The user has not explicitly selected a profile.

---

## 23. Lock-validation and abuse-resistance test plan

This section verifies that BusSec can be used to test owner-controlled targets without turning the normal firmware into a general extraction appliance.

### 23.1 Test target classes

| Target class | Description | Purpose |
|---|---|---|
| TARGET_A_UNLOCKED | Fresh unlocked AVR UPDI target. | Confirm normal attach/read/program behavior. |
| TARGET_B_LOCKED | Same family, programmed and locked. | Confirm protected readout is denied or reported as failure if it succeeds. |
| TARGET_C_BAD_VTGT | Missing or invalid target voltage. | Confirm BusSec refuses to drive. |
| TARGET_D_HV_DUMMY | HV dummy load, not an MCU. | Confirm HV dry-run and interlocks. |
| TARGET_E_SACRIFICIAL | Cheap sacrificial AVR. | Destructive programming/fuse/HV testing only. |
| TARGET_F_BUSSEC_LOCKED | BusSec board itself after production lock. | Confirm BusSec firmware is not casually readable or replaceable. |

### 23.2 LOCKTEST matrix

| Test ID | Target state | Action | Expected result |
|---|---|---|---|
| LB-001 | Unlocked | `locktest.begin` | Session starts with owned-target confirmation. |
| LB-002 | Unlocked | Read signature/device ID | Succeeds. |
| LB-003 | Unlocked | Read lock/security status | Reports unlocked. |
| LB-004 | Unlocked | Try flash read | Succeeds or is classified according to profile. |
| LB-005 | Locked | `locktest.begin` | Session starts with owned-target confirmation. |
| LB-006 | Locked | Read signature/device ID | Succeeds where silicon permits. |
| LB-007 | Locked | Read lock/security status | Reports locked/protected. |
| LB-008 | Locked | Try flash read | Attempt is made through documented interface and classified. Expected PASS result is denied. |
| LB-009 | Locked | Try EEPROM/user-row/config read | Attempt is made only if profile permits; expected PASS result is denied if protected. |
| LB-010 | Locked | Try bootloader read | Expected PASS result is denied if bootloader honors protection. |
| LB-011 | Locked | Reset-repeat and retry | Protection state persists. |
| LB-012 | Locked | Powercycle-repeat and retry | Protection state persists. |
| LB-013 | Locked | Try malformed read alias | Rejected or routed to same policy gate. |
| LB-014 | Locked | Confirmation token supplied to normal read command | Still refused outside LOCKTEST. |
| LB-015 | Locked | End LOCKTEST | Report states destructive_actions=none. |

### 23.3 Abuse-resistance matrix

| Test ID | Condition | Action | Expected result |
|---|---|---|---|
| AR-001 | NORMAL mode | Try protected flash read | Refused with hint to use LOCKTEST for owned validation. |
| AR-002 | PROGRAM mode | Try protected flash read | Refused. Program mode may erase/write only with confirmation. |
| AR-003 | LOCKTEST mode | Try protected flash read | Attempted and classified. |
| AR-004 | No owned-target confirmation | Start LOCKTEST | Refused. |
| AR-005 | Invalid VTGT | Any bus-driving action | Refused. |
| AR-006 | Unknown device profile | Memory read/write | Refused or limited to ID/status. |
| AR-007 | Raw bridge disabled | Start raw bridge | Refused in PUBLIC/LAB build. |
| AR-008 | Safe bridge enabled | Request transcript off | Rejected or transcript remains forced on. |
| AR-009 | HV board absent | HV activate | Refused. |
| AR-010 | HV arm jumper absent | HV activate | Refused. |
| AR-011 | Fuzz default profile | NVM command emission | Not possible. |
| AR-012 | Destructive fuzz profile | No compile flag | Refused. |

### 23.4 Bridge-mode policy

Bridge mode must not silently bypass the policy layer in release builds.

| Bridge mode | Availability | Policy |
|---|---|---|
| SAFE_BRIDGE | LAB/PUBLIC if implemented | Transcript forced on; protected readout attempts logged or blocked according to profile. |
| RAW_BRIDGE | DEV only | Requires `ENABLE_RAW_BRIDGE=1`; emits clear audit banner. |

Required bridge tests:

```text
BR-001 SAFE_BRIDGE refuses to start without VTGT.
BR-002 SAFE_BRIDGE logs session start and target voltage.
BR-003 SAFE_BRIDGE cannot disable transcript logging in PUBLIC/LAB builds.
BR-004 RAW_BRIDGE is unavailable unless compiled in.
BR-005 RAW_BRIDGE prints build-mode and audit warning before activation.
```

### 23.5 BusSec self-protection tests

For release or production-test units, validate the BusSec board itself:

```text
1. Program official BusSec firmware.
2. Verify firmware hash.
3. Configure BusSec MCU security/readout protection according to PIC32CM documentation.
4. Power cycle.
5. Confirm normal CLI still works.
6. Attempt BusSec firmware readout over SWD using normal tools.
7. Confirm readout is blocked or restricted as intended.
8. Confirm firmware update path requires signed image or physical service mode if implemented.
```

Acceptance:

```text
SELF-001 BusSec firmware readout blocked after production lock.
SELF-002 Debug attach blocked or restricted after production lock.
SELF-003 Normal CLI and bus functions still work.
SELF-004 Firmware update without service authorization is denied.
SELF-005 Unsigned update image is denied if update support exists.
```

### 23.6 Required JSON report shape

```json
{
  "tool": "BusSec-PIC32CM",
  "tool_fw": "v0.4-locktest",
  "build_mode": "BUSSEC_LAB",
  "target": "ATmega4809",
  "signature": "1E9651",
  "interface": "UPDI",
  "detected_state": "locked",
  "attempts": [
    {
      "operation": "try-flash-read",
      "address": "0x0000",
      "length": 256,
      "result": "denied-by-target",
      "verdict": "PASS"
    }
  ],
  "destructive_actions_performed": false,
  "final_verdict": "PASS"
}
```

### 23.7 Acceptance criteria

```text
[ ] NORMAL mode refuses protected readout.
[ ] LOCKTEST mode can attempt documented protected readout on owned targets.
[ ] LOCKTEST denial is treated as PASS for locked targets.
[ ] LOCKTEST successful readout on a supposedly locked target is treated as FAIL.
[ ] Confirmation tokens cannot make NORMAL mode perform protected readout.
[ ] All memory operations pass through a common policy gate.
[ ] HV cannot fire without compile flag, board ID, arm jumper, VTGT, isolation, and token.
[ ] Fuzzing cannot emit destructive or protected-memory commands by default.
[ ] Raw bridge is unavailable in release builds.
[ ] BusSec itself can be locked against casual firmware readout/replacement.
```

---

## 24. Self-test plan

Firmware commands:

```text
selftest clock
selftest ram
selftest host-uart
selftest uart-loopback A
selftest uart-loopback B
selftest i2c-pins
selftest spi-loopback
selftest reset A
selftest trigger
selftest vtgt
selftest updi-loopback
selftest updi-vtgt
selftest updi-hv-dryrun
```

### 24.1 UPDI loopback jig

A UPDI self-test jig can include:

- UPDI_DATA looped through a resistor network
- Optional AVR sacrificial target socket
- VTGT source selector: 3.3 V / 5 V
- HV dummy load on HV board variant

### 24.2 HV dry run

HV dry run must not connect to a real AVR.

It verifies:

- HV rail enable/disable
- HV switch gate control
- normal UPDI isolation
- bleed/discharge behavior
- ADC thresholds
- emergency off

---

## 25. Bring-up plan

Do not skip stages.

| Stage | Goal | Gate |
|---:|---|---|
| 0 | Build/link/startup | Vector table valid, reset handler runs. |
| 1 | LED blink / GPIO | GPIO toggles at expected rate. |
| 2 | Clock verify | Measured clock matches config. |
| 3 | Host UART echo | CLI accepts commands. |
| 4 | Logging core | 1 Hz status with monotonic timestamp. |
| 5 | VTGT sense | ADC reports known voltages correctly. |
| 6 | UART monitor | 1 MB at 115200, zero loss. |
| 7 | UART DMA | High-rate capture, drops understood. |
| 8 | UART inject | Loopback exact bytes. |
| 9 | UART bridge | Two endpoints byte-perfect at 115200. |
| 10 | Reset pulse | Known dev board resets. |
| 11 | I²C scan | Known devices found. |
| 12 | I²C recovery | Stuck SDA handled. |
| 13 | SPI xfer | W25Q JEDEC ID read. |
| 14 | Binary log parser | Host parser resyncs after corruption. |
| 15 | UPDI electrical idle | UPDI line safe, VTGT gates drive. |
| 16 | UPDI bridge | Host tool talks through BusSec to known AVR. |
| 17 | UPDI native attach | Signature read works. |
| 18 | UPDI native flash | Program sacrificial AVR and verify. |
| 19 | UPDI fuse read | Fuse decode works. |
| 20 | UPDI fuse write dry-run | Diff and refusal paths work. |
| 21 | LOCKTEST unlocked target | Read attempts classified correctly. |
| 22 | LOCKTEST locked target | Protected readout attempts are denied or reported as target failure if they succeed. |
| 23 | LOCKTEST report | JSON/text report generated with destructive_actions=none. |
| 24 | UPDI HV dummy load | HV dry-run passes. |
| 25 | UPDI HV real target | Owned shared-UPDI AVR re-enabled. |
| 26 | Backpressure tests | Drop policy behaves correctly. |
| 27 | v2 fuzzers | Only after v1.x is stable. |

---

## 26. Test equipment

Minimum:

| Item | Spec | Purpose |
|---|---|---|
| Logic analyzer | ≥24 MS/s, 8 channels | UART/I²C/SPI/UPDI timing truth. |
| DMM | basic | Rails, reset, VTGT, HV sanity checks. |
| Known I²C devices | EEPROM, sensor, display | Scanner/read-write tests. |
| Known SPI flash | W25Q/MX25 class | SPI transaction tests. |
| USB-UART or second MCU | 3.3/5 V | UART monitor/bridge tests. |
| Known UPDI AVR | ATtiny817 / ATmega4809 / AVR-DA/DB style | UPDI tests. |

Useful:

| Item | Spec | Purpose |
|---|---|---|
| Oscilloscope | ≥100 MHz | Edge quality, HV pulse validation, level shifting. |
| Variable PSU | 1.8/3.3/5 V | Target-voltage compatibility. |
| Current-limited bench PSU | Adjustable | Sacrificial target work. |
| Spare sacrificial AVR boards | cheap | UPDI recovery/fuse/HV testing. |

---

## 27. Risk register

| Risk | Likelihood | Impact | Mitigation |
|---|---:|---:|---|
| Wrong level shifting damages target or tool | Medium | High | VTGT sense, translators, current limits. |
| HV UPDI damages target-side circuitry | Medium | High | HV only on isolated board, explicit warnings, dummy-load testing. |
| Firmware drives UPDI during target response | Medium | Medium | Strict half-duplex state machine, RX echo monitor. |
| UPDI fuse write disables future access | Medium | Medium | Dry-run diff, explicit confirmation, known-good profiles. |
| Host logging cannot keep up | Medium | Medium | Binary mode and backpressure. |
| SRAM exhaustion | Medium | High | Build-time RAM limit and no heap. |
| DMA conflicts | Medium | Medium | Mode-dependent DMA allocator. |
| Timestamp claims overstated | Medium | Low | Document actual timing limits. |
| Scope creep | High | High | Keep v1 basic; defer fuzz/MITM. |
| Using stale silicon assumptions | Medium | Medium | Check current data sheet and errata before PCB/final firmware. |

---

## 28. PCB design notes

### 28.1 Connectors

Recommended front panel:

```text
HOST USB / debugger connector
UART A 6-pin
UART B 6-pin
I²C 6-pin
SPI 8-pin
UPDI 6-pin
RESET/POWER auxiliary header
TRIGGER header
```

### 28.2 Silkscreen rules

Clearly mark:

- GND
- VTGT
- signal direction
- voltage domain
- HV capable / not HV capable
- UPDI normal vs HV connector
- target power output current limit

### 28.3 Jumpers

Useful jumpers:

- Target power enable
- I²C pull-up enable
- UPDI simple mode vs translated mode
- HV hardware arm
- Bootloader/debug mode
- Host VCP selection

### 28.4 PCB safety around HV_UPDI

For HV-capable variant:

- Keep HV trace away from normal GPIO traces.
- Use clearance appropriate for the selected HV level.
- Put HV components near the UPDI connector.
- Add test point for HV rail.
- Add bleed resistor.
- Add a visible HV armed LED or indicator.
- Keep normal translator protected by analog isolation.

---

## 29. Recommended first hardware revision

Do **not** build every optional feature into rev A.

### 29.1 Rev A: non-HV board

Include:

- PIC32CM5164JH01048
- Host serial/debug connector
- UART A/B level shifting
- I²C level shifting and pull-ups
- SPI level shifting
- UPDI normal 1-wire front-end
- VTGT sense for all ports
- Reset MOSFETs
- Trigger outputs
- Optional target VCC switches

Exclude:

- HV UPDI
- Power analysis
- Glitching
- Complex analog measurement

### 29.2 Rev B: HV UPDI board or add-on

Make HV a separate revision or plug-on module.

Reasons:

- HV mistakes destroy boards.
- Normal UPDI programming is useful without HV.
- Isolation and safety are easier to validate separately.

---

## 30. Documentation deliverables

Repository:

```text
README.md
  purpose
  safety/use policy
  quick start
  wiring diagrams
  build instructions
  command examples

/docs
  hardware_frontend.md
  uart_tool.md
  i2c_tool.md
  spi_tool.md
  updi_tool.md
  updi_hv.md
  logging_format.md
  host_protocol.md
  troubleshooting.md
  test_plan.md
  locktest_validation.md
  abuse_resistance.md

/hardware
  schematic
  PCB
  BOM
  front-panel pinout
  HV add-on schematic if used

/firmware
  source
  startup
  linker script
  Makefile/CMake

/host
  Python CLI
  log parsers
  UPDI helper
  replay tools
  tests
```

---

## 31. Implementation order for UPDI

Practical path:

1. Build normal UPDI electrical front-end.
2. Verify line idle level and VTGT ADC.
3. Implement raw byte TX/RX with echo monitor.
4. Implement bridge mode.
5. Test bridge with a known host tool and sacrificial AVR.
6. Log raw UPDI bytes.
7. Implement native attach/read-signature.
8. Implement host-side device database.
9. Implement flash erase/write/verify on sacrificial AVR.
10. Implement fuse read.
11. Implement fuse write dry-run.
12. Implement controlled fuse write.
13. Only then design HV add-on.
14. Validate HV add-on on dummy load.
15. Validate HV add-on on sacrificial shared-UPDI AVR.

---

## 32. References to consult during implementation

Primary references:

- Microchip PIC32CM5164JH01048 product page
- Microchip PIC32CM JH00/JH01 family data sheet
- Microchip PIC32CM JH00/JH01 silicon errata
- Microchip PIC32CM-JH DFP pack contents
- Microchip UPDI documentation
- Target AVR data sheets for exact UPDI behavior, memory maps, fuses, and NVM operation
- pyupdi source and documentation for simple serial electrical interface behavior
- pymcuprog source and documentation for supported programmer flows
- NXP UM10204 I²C-bus specification
- Winbond W25Q-series data sheet for SPI flash testing

Useful public starting points:

- Microchip UPDI high-voltage activation information
- Microchip debugging with UPDI guide
- mraardvark/pyupdi
- microchip-pic-avr-tools/pymcuprog

---

## 33. Final v0.4 design decision list

1. Primary custom-board MCU changed to PIC32CM5164JH01048.
2. Original UART/I²C/SPI scope preserved.
3. UPDI added as a first-class bus family.
4. UPDI bridge mode is the first UPDI milestone.
5. Native UPDI programming is second UPDI milestone.
6. UPDI HV activation is optional and deferred to a separate hardware variant.
7. Protected target readout is not a normal user feature; owner-authorized LOCKTEST mode may attempt documented readout paths and classify results.
8. Chip erase is allowed only with explicit owned-target confirmation.
9. Fuse writes require diff and confirmation.
10. UPDI fuzzing is constrained and deferred to v2.
11. No RTOS in v1.
12. No heap in firmware runtime paths.
13. Host handles Intel HEX and device database.
14. Firmware handles low-level timing, line state, logging, and safety gates.
15. All target-facing drive operations are gated by VTGT sensing.
16. HV path must be physically isolated and explicitly armed.
17. Rev A should omit HV and prove normal bus functions first.
18. Rev B or add-on can handle HV UPDI.
19. LOCKTEST mode is a first-class validation feature, separate from NORMAL, PROGRAM, and RESEARCH_DEV modes.
20. LOCKTEST denial is PASS for locked targets; successful readout on a supposedly locked target is FAIL.
21. BusSec production firmware should be lock-validated against casual readout/replacement.

# BusSec-PIC32CM Instructions

## Curiosity Nano CLI

The current bring-up firmware uses the PIC32CM JH01 Curiosity Nano+ Touch
on-board debugger CDC virtual COM port as the host CLI.

Serial settings:

```text
115200 baud
8 data bits
no parity
1 stop bit
no flow control
```

The current Curiosity Nano bring-up console receives with SERCOM1 on PA00 and
transmits with a blocking GPIO software UART on PA01. The software TX path keeps
the board's fixed debugger CDC wiring usable even though the PIC32CM USART cannot
route hardware TX to SERCOM1 PAD1.

On Linux, the board usually appears as `/dev/ttyACM0`, `/dev/ttyACM1`, etc.
Check which device appears after plugging in the board:

```bash
ls /dev/ttyACM*
```

Connect with `tio`:

```bash
tio /dev/ttyACM0 -b 115200
```

Or connect with `picocom`:

```bash
picocom -b 115200 /dev/ttyACM0
```

After flashing the firmware and opening the serial port, expected startup text:

```text
# BusSec-PIC32CM v0.1-alpha
# Type help
```

Initial commands:

```text
help
version
status
research status
research begin confirm=SACRIFICIAL_TARGET
research scan-safe
research end
policy selftest
```

The firmware also emits a `STATUS ...` line once per second.

The current `research scan-safe` command is a policy-gated bring-up stub. It
does not drive UPDI yet; it reports that the UPDI transaction layer is missing
and confirms that protected and undocumented readout paths stay blocked.

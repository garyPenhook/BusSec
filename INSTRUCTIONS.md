# BusSec-PIC32CM Instructions

## Curiosity Nano CLI

The current bring-up firmware uses the PIC32CM JH01 Curiosity Nano+ Touch
on-board debugger CDC virtual COM port as the host CLI.

Serial settings:

```text
921600 baud
8 data bits
no parity
1 stop bit
no flow control
```

On Linux, the board usually appears as `/dev/ttyACM0`, `/dev/ttyACM1`, etc.
Check which device appears after plugging in the board:

```bash
ls /dev/ttyACM*
```

Connect with `tio`:

```bash
tio /dev/ttyACM0 -b 921600
```

Or connect with `picocom`:

```bash
picocom -b 921600 /dev/ttyACM0
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
```

The firmware also emits a `STATUS ...` line once per second.

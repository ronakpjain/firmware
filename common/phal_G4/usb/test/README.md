# STM32G4 USB host echo test

This Rust program talks to the USB device produced by the G4 hardware test in
`source/g4_testing/usb.c`. That firmware exposes a vendor-class bulk interface:

- VID: `0x1209`
- PID: `0x0003`
- Interface: `0`
- OUT endpoint: `0x01`
- IN endpoint: `0x81`
- Maximum packet size: `64` bytes

The program sends packets to the OUT endpoint, reads the echoed packet from the
IN endpoint, and compares every byte.

## Run

Install `libusb` for the host platform, then run from this directory:

```sh
cargo run --release
```

Optional positional arguments are:

```text
cargo run --release -- <iterations> <packet-length> <timeout-ms>
```

Defaults are `100` iterations, `64`-byte packets, and a `1000` ms timeout.
Packet lengths from 1 through 64 bytes are supported because the firmware test
handles one full-speed bulk packet at a time.

The host may need permission to access the USB device. On Linux, add an
appropriate udev rule or run the test with the required permissions.

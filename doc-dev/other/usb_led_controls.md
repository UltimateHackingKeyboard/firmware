# USB LED Controls

This document describes USB commands for controlling LEDs and displays on UHK keyboards.

For easy testing, we include an `UHKID` argument that can be used to target a specific UHK in a multi-UHK environment. Feel free to set it to empty string

```bash
UHKID=--serial-number=1777777765
```

## LED Override Mask (0x0a)

Get/set override flags for LEDs. The mask controls which LED subsystems are in override mode.

**Get current mask:**

```bash
./send-command.ts $UHKID 0x12 0x0a
```

**Set mask:**
```bash
./send-command.ts $UHKID 0x13 0x0a 255 255 128 17
```

Format: `0x13 0x0a <uhk60_flags> <oled_mode> <per_key_bitmap...>`

- Byte 1: UHK60 LED override flags (bitfield: mod, fn, mouse, capsLock, agent, adaptive, segmentDisplay)
- Byte 2: OLED override mode
- Bytes 3-34: Per-key RGB override bitmap (256 bits for key IDs)

## UHK60 LED Panel (0x20)

Set UHK60 indicator LEDs and 14-segment display.

```bash
./send-command.ts $UHKID 0x20 0 1 0 1 0 1  0 0 0 0 1 0 0 0 0 0 0 1 0 0  0 0 1 0 1 0 0 0 0 0 0 0 0 1  0 0 1 0 0 0 0 0 0 1 0 0 0 0
```

Format: `0x20 <6 LED bytes> <42 segment bytes>`

- Bytes 1-6: LED brightness (mod, fn, mouse, capsLock, agent, adaptive)
- Bytes 7-48: 14-segment display (3 chars × 14 segments)

Reset override:
```bash
./send-command.ts $UHKID 0x13 0x0a 0 0 0 0
```

## Per-Key RGB (0x21)

Set individual key colors.

```bash
# First enable per-key override in mask
./send-command.ts $UHKID 0x13 0x0a 0 0 255 255 255 255 255 255 255

# Set key 5 to red, key 10 to green
./send-command.ts $UHKID 0x21  2  5 255 0 0  10 0 255 0
```

Format: `0x21 <count> [<keyId> <r> <g> <b>]...`

- Byte 1: Number of keys to set
- Then for each key: keyId (slot×64 + inSlotIdx), R, G, B values

Reset override:
```bash
./send-command.ts $UHKID 0x13 0x0a 000 000 000 000 000 000 000 000 000
```

## OLED Display (0x15)

Draw pixels on the UHK80 OLED (256×64, 4-bit grayscale).

**Enable OLED override:**
```bash
./send-command.ts $UHKID 0x13 0x0a 0 255 0
```

**Show endianity:**
```bash
./send-command.ts $UHKID 0x15 128 32 8 0xFF 0x0F 0xF0 0xFF  255
```

**Draw a heart:**
```bash
./send-command.ts $UHKID 0x15 \
  8 13 6 0x0f 0xf0 0xff \
  8 14 7 0xf0 0x0f 0x00 0xf0 \
  8 15 6 0x0f 0x00 0x0f \
  8 16 6 0x00 0xf0 0xf0 \
  8 17 6 0x00 0x0f 0x00 \
  0xff
```

Format: `0x15 [<x> <y> <pixelCount> <packed_pixels>...]... 0xff`

- Each segment: x, y, pixel count, then packed pixel data
- Pixels are 4-bit grayscale (0x0=black, 0xf=white), 2 per byte (high nibble first)
- Terminate with x=0xff

# K-Boot Protocol

This document describes the K-boot bootloader protocol used for flashing module firmware. The protocol is currently implemented in the Agent (TypeScript) and needs to be ported to the right half firmware.

## Protocol Overview

K-boot is NXP's bootloader protocol for Kinetis MCUs. The UHK uses it for:
- Direct USB flashing of the right half (KL04 bootloader)
- I2C-forwarded flashing of modules via Buspal mode

## Command IDs

From `lib/agent/packages/kboot/src/enums/commands.ts`:

| ID | Command | Description |
|----|---------|-------------|
| 0x01 | FlashEraseAll | Erase entire flash |
| 0x02 | FlashEraseRegion | Erase region (start + count) |
| 0x03 | ReadMemory | Read from address |
| 0x04 | WriteMemory | Write to address (has data phase) |
| 0x05 | FillMemory | Fill memory region |
| 0x06 | FlashSecurityDisable | Unlock flash (8-byte key) |
| 0x07 | GetProperty | Query bootloader property |
| 0x0B | Reset | Reset to firmware |
| 0x0C | SetProperty | Set bootloader property |
| 0x0D | FlashEraseAllUnsecure | Erase all (unsecured) |
| 0xC1 | ConfigureI2c | Configure I2C for buspal |

## Packet Structure

### Command Packet (32 bytes)

```
Header (4 bytes):
  [0] = 1                      // Channel (always 1 for commands)
  [1] = 0                      // Reserved
  [2-3] = Payload length       // Little-endian 16-bit

Payload (variable):
  [0] = Command ID
  [1] = HasDataPhase flag      // 1 if data follows, 0 otherwise
  [2] = 0                      // Reserved
  [3] = Param count            // Number of 4-byte parameters
  [4+] = Parameters            // 32-bit little-endian each

Padding: Fill to 32 bytes with zeros
```

### Response Packet

```
Header (4 bytes):
  [0] = 3                      // Channel (always 3 for response)
  [1] = Reserved
  [2-3] = Response data length

Response Data:
  [4] = ResponseTag            // 0xA0=generic, 0xA3=readMem, 0xA7=property
  [5-7] = Reserved
  [8-10] = ResponseCode        // 24-bit little-endian status
  [11+] = Response-specific data
```

### Data Packet (WriteMemory data phase)

```
[0] = 2                        // Channel 2 (data)
[1] = 0
[2] = Data chunk length        // 1-28 bytes typically
[3] = 0
[4+] = Actual firmware data
```

## Response Tags

| Tag | Description |
|-----|-------------|
| 0xA0 | Generic response |
| 0xA3 | ReadMemory response |
| 0xA7 | Property response |
| 0xAF | FlashReadOnce response |

## Response Codes

| Code | Description |
|------|-------------|
| 0 | Success |
| 1 | Fail |
| 2 | ReadOnly |
| 3 | OutOfRange |
| 4 | InvalidArgument |
| 100-106 | Flash driver errors |
| 200-202 | I2C driver errors |
| 10000-10005 | Bootloader errors |

## Properties

| ID | Property |
|----|----------|
| 0x01 | BootloaderVersion |
| 0x03 | FlashStartAddress |
| 0x04 | FlashSize |
| 0x05 | FlashSectorSize |
| 0x0B | MaxPacketSize |
| 0x0E | RAMStartAddress |
| 0x0F | RAMSize |
| 0x11 | FlashSecurityState |

## Flashing State Machine

### Right Half (Direct USB)

1. Reenumerate device into bootloader mode
2. Create KBoot USB peripheral instance
3. `flashSecurityDisable([0x01-0x08])` - Unlock flash
4. `flashEraseRegion(0xC000, 475136)` - Erase application region
5. Read firmware from .hex file
6. `writeMemory(startAddress, data)` - Write in chunks
7. `reset()` - Reboot to firmware

### Module (I2C via Buspal)

1. Reenumerate to NormalKeyboard mode
2. Send ping to module via I2C (100 retries)
3. Send jump-to-bootloader to module
4. Poll CurrentKbootCommand until idle
5. Reenumerate into Buspal mode
6. `configureI2c(moduleAddress, speed=64)`
7. `flashEraseAllUnsecure()`
8. Read firmware from binary file
9. `writeMemory(0, data)`
10. `reset()`
11. Reenumerate back to normal keyboard
12. Send reset command to module
13. Send idle command to module

## Module I2C Addresses (Buspal)

From `lib/agent/packages/uhk-common`:

| Module | Address |
|--------|---------|
| Left Half | 0x40 |
| Key Cluster Left | 0x41 |
| Trackball Right | 0x42 |
| Trackpoint Right | 0x43 |
| Touchpad Right | 0x44 |

## WriteMemory Two-Phase Protocol

1. **Command phase**: Send WriteMemory with address and total size
2. **Data phase**: Send data in 32-byte HID packets (channel 2)
3. **Response phase**: Receive confirmation after all data sent

## Timeout Handling

- Response timeout: 2000 ms
- USB read timeout: 1000 ms
- Buspal connection retries: 30 seconds with 2-second intervals

## Agent Implementation Files

| File | Purpose |
|------|---------|
| `lib/agent/packages/kboot/src/kboot.ts` | Main KBoot class |
| `lib/agent/packages/kboot/src/usb-peripheral.ts` | USB peripheral |
| `lib/agent/packages/kboot/src/util/usb/encode-command-option.ts` | Command encoding |
| `lib/agent/packages/kboot/src/util/usb/decode-command-response.ts` | Response parsing |
| `lib/agent/packages/uhk-usb/src/uhk-operations.ts` | Firmware update workflows |

## Porting Considerations

### For Firmware Implementation

1. **Packet Encoding**: Implement 32-byte command packet builder
2. **Response Parsing**: Handle 0xA0/0xA3/0xA7 response tags
3. **Data Phase**: Support multi-packet data transfer
4. **I2C Transport**: Replace USB with I2C for module communication
5. **State Machine**: Implement erase → write → reset sequence
6. **Error Handling**: Handle I2C timeouts and retry logic

### Memory Requirements

- Command buffer: 32 bytes
- Response buffer: 32 bytes
- Data chunk buffer: 32 bytes
- Firmware staging: Depends on approach (streaming vs buffered)

### Key Differences from Agent

1. No USB layer (direct I2C to module bootloader)
2. No async/await (use callbacks or blocking I2C)
3. Limited RAM (streaming approach preferred)
4. CRC verification may be needed for I2C reliability

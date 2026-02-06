# Firmware Upload Proposal

## Current UserConfig Flashing Flow

```
Agent                                    Firmware
  |                                         |
  |  1. WriteStagingUserConfig (0x06)       |
  |     [cmd, len, offsetLo, offsetHi, data...]
  |  ---------------------------------------->
  |     (repeat for each 59-byte chunk)     |
  |                                         |
  |  2. ApplyConfig (0x07)                  |
  |  ---------------------------------------->
  |     Validates & swaps staging→validated |
  |                                         |
  |  3. LaunchEepromTransfer (0x08)         |
  |     [cmd, operation=write, bufferId]    |
  |  ---------------------------------------->
  |     Writes validated buffer to EEPROM   |
  |                                         |
  |  4. GetDeviceState (0x09) - poll        |
  |  ---------------------------------------->
  |  <-- isEepromBusy until done            |
```

### USB Packet Format (63 bytes max)

```
WriteConfig packet:
  [0] = UsbCommand (0x05 or 0x06)
  [1] = length (1-59)
  [2] = offset low byte
  [3] = offset high byte
  [4-62] = data (up to 59 bytes)
```

### Agent Code Reference

```typescript
// lib/agent/packages/uhk-usb/src/util.ts
function getTransferBuffers(usbCommand, configBuffer) {
    const MAX_SENDING_PAYLOAD_SIZE = 59;  // 63 - 4 byte header
    for (let offset = 0; offset < configBuffer.length; offset += 59) {
        const header = [usbCommand, length, offset & 0xFF, offset >> 8];
        fragments.push(concat(header, configBuffer.slice(offset, offset + 59)));
    }
}

// lib/agent/packages/uhk-usb/src/uhk-operations.ts
async saveUserConfiguration(buffer) {
    await sendConfigToKeyboard(buffer, true);      // WriteStagingUserConfig chunks
    await applyConfiguration();                     // ApplyConfig
    await writeConfigToEeprom(validatedUserConfig); // LaunchEepromTransfer
    await waitUntilKeyboardBusy();                  // Poll GetDeviceState
}
```

## Proposed Firmware Upload API

Reuse the same chunked transfer pattern, but target module firmware instead of config buffers.

### New USB Commands

| ID | Command | Parameters |
|----|---------|------------|
| 0x20 | WriteModuleFirmware | [len, offsetLo, offsetHi, data...] |
| 0x21 | FlashModule | [slotId] |
| 0x22 | GetModuleFlashState | - |

### Proposed Flow

```
Agent                                    Firmware
  |                                         |
  |  1. WriteModuleFirmware (0x20)          |
  |     [cmd, len, offsetLo, offsetHi, data...]
  |  ---------------------------------------->
  |     (repeat for each 59-byte chunk)     |
  |     Streams directly to module via I2C  |
  |                                         |
  |  2. FlashModule (0x21)                  |
  |     [cmd, slotId]                       |
  |  ---------------------------------------->
  |     Finalizes flash, resets module      |
  |                                         |
  |  3. GetModuleFlashState (0x22) - poll   |
  |  ---------------------------------------->
  |  <-- status: idle/erasing/writing/done  |
```

### Implementation Options

**Option A: Streaming (preferred for UHK60)**
- Each WriteModuleFirmware chunk is immediately forwarded to module via I2C
- No RAM buffer needed on right half
- Module bootloader receives K-boot WriteMemory commands directly

**Option B: Buffered (if streaming not possible)**
- Store firmware in staging buffer (limited to ~32KB)
- FlashModule reads from buffer and sends to module
- Requires multiple passes for larger firmware

### Agent Code Changes

```typescript
// Minimal changes - reuse existing pattern
async flashModuleFirmware(slotId: number, firmwareBuffer: Buffer) {
    // Same chunked transfer, different command
    const fragments = getTransferBuffers(0x20, firmwareBuffer);
    for (const fragment of fragments) {
        await this.device.write(fragment);
    }

    // Trigger flash
    await this.device.write(Buffer.from([0x21, slotId]));

    // Wait for completion
    while (true) {
        const state = await this.device.write(Buffer.from([0x22]));
        if (state[1] === FlashState.Done) break;
        if (state[1] === FlashState.Error) throw new Error(state[2]);
        await snooze(100);
    }
}
```

### Firmware Side Implementation

```c
// New command handlers in usb_commands/

void UsbCommand_WriteModuleFirmware(const uint8_t *in, uint8_t *out) {
    uint8_t length = GetUsbRxBufferUint8(1);
    uint16_t offset = GetUsbRxBufferUint16(2);
    const uint8_t *data = in + 4;

    // Stream to module bootloader via I2C (K-boot WriteMemory)
    KbootDriver_WriteMemory(offset, data, length);
}

void UsbCommand_FlashModule(const uint8_t *in, uint8_t *out) {
    uint8_t slotId = GetUsbRxBufferUint8(1);

    // Send K-boot Reset command to module
    KbootDriver_Reset(slotId);
}
```

## Summary

The existing config transfer infrastructure (chunked USB, 59-byte payloads, 16-bit offset) can be reused directly for firmware upload. The main work is:

1. Add 3 new USB command handlers
2. Implement K-boot I2C driver (WriteMemory, Reset commands)
3. Handle the streaming/forwarding in firmware instead of Agent

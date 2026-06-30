# USB Communication APIs

This document describes the USB command infrastructure used for host-to-device communication.

## USB Command Handling

### Main Protocol Handler

**File**: `right/src/usb_protocol_handler.c`

The `UsbProtocolHandler()` function is the central dispatcher that:
- Receives command byte from position 0 in RX buffer
- Routes to appropriate command handler via switch statement
- Clears RX buffer after processing

### Command IDs

From `right/src/usb_protocol_handler.h`:

```c
typedef enum {
    UsbCommandId_GetDeviceProperty        = 0x00,
    UsbCommandId_Reenumerate              = 0x01,
    UsbCommandId_JumpToModuleBootloader   = 0x02,
    UsbCommandId_SendKbootCommandToModule = 0x03,
    UsbCommandId_ReadConfig               = 0x04,
    UsbCommandId_WriteHardwareConfig      = 0x05,
    UsbCommandId_WriteStagingUserConfig   = 0x06,
    UsbCommandId_ApplyConfig              = 0x07,
    UsbCommandId_LaunchStorageTransfer    = 0x08,
    UsbCommandId_GetDeviceState           = 0x09,
    // ... more commands up to 0x1f
} usb_command_id_t;
```

## Buffer Specifications

From `right/src/usb_interfaces/usb_interface_generic_hid.h`:

```c
#define USB_GENERIC_HID_INTERRUPT_IN_PACKET_SIZE  63
#define USB_GENERIC_HID_INTERRUPT_OUT_PACKET_SIZE 63
#define USB_GENERIC_HID_IN_BUFFER_LENGTH          63
#define USB_GENERIC_HID_OUT_BUFFER_LENGTH         63
```

- **Packet size**: 63 bytes (USB interrupt endpoint)
- **Available payload**: 62 bytes (after status code)

## Buffer Access Macros

From `right/src/usb_protocol_handler.h`:

```c
#define GetUsbRxBufferUint8(OFFSET)  (GetBufferUint8(GenericHidOutBuffer, OFFSET))
#define GetUsbRxBufferUint16(OFFSET) (GetBufferUint16(GenericHidOutBuffer, OFFSET))
#define GetUsbRxBufferUint32(OFFSET) (GetBufferUint32(GenericHidOutBuffer, OFFSET))

#define SetUsbTxBufferUint8(OFFSET, VALUE)  (SetBufferUint8(GenericHidInBuffer, OFFSET, VALUE))
#define SetUsbTxBufferUint16(OFFSET, VALUE) (SetBufferUint16(GenericHidInBuffer, OFFSET, VALUE))
#define SetUsbTxBufferUint32(OFFSET, VALUE) (SetBufferUint32(GenericHidInBuffer, OFFSET, VALUE))
```

## Chunked Transfer Pattern

Large data transfers use a 4-byte header followed by data:

```
Header (4 bytes):
  [0] = Command ID
  [1] = Length
  [2] = Offset low byte
  [3] = Offset high byte

Data (up to 59 bytes):
  [4+] = Payload
```

This allows transfers up to 64 KB using 16-bit offset addressing.

## Status Codes

### General Status Codes

```c
typedef enum {
    UsbStatusCode_Success        = 0,
    UsbStatusCode_InvalidCommand = 1,
    UsbStatusCode_Busy           = 2,
} usb_status_code_general_t;
```

### Command-Specific Status Codes

Each command defines its own status enum. Example from read config:

```c
typedef enum {
    UsbStatusCode_ReadConfig_InvalidConfigBufferId = 2,
    UsbStatusCode_ReadConfig_LengthTooLarge        = 3,
    UsbStatusCode_ReadConfig_BufferOutOfBounds     = 4,
} usb_status_code_read_config_t;
```

## Existing Module-Related Commands

### JumpToModuleBootloader (0x02)

```
Request:
  [0] = 0x02
  [1] = Module slot ID

Response:
  [0] = Status code
```

### SendKbootCommandToModule (0x03)

```
Request:
  [0] = 0x03
  [1] = K-boot command (0=idle, 1=ping, 2=reset)
  [2] = Module I2C address (if command != idle)

Response:
  [0] = Status code
```

## Agent-Side Implementation

### USB Device Wrapper

**File**: `lib/agent/packages/uhk-usb/src/uhk-hid-device.ts`

```typescript
public async write(buffer: Buffer): Promise<Buffer> {
    const device = await this.getDevice();
    await device.write(sendData);
    let receivedData = await device.read(1000);
    if (receivedData[0] !== 0) {
        throw new Error(`Response code: ${receivedData[0]}`);
    }
    return Buffer.from(receivedData);
}
```

### Fragment Generation

**File**: `lib/agent/packages/uhk-usb/src/util.ts`

```typescript
export function getTransferBuffers(usbCommand: UsbCommand, configBuffer: Buffer): Buffer[] {
    const MAX_SENDING_PAYLOAD_SIZE = MAX_USB_PAYLOAD_SIZE - 4;  // 59 bytes
    // Creates array of 63-byte buffers with header + data chunks
}
```

## Extension Points for Firmware Upload

1. **New USB Command IDs**: Reserve 0x20-0x2f for firmware operations
2. **Chunked Transfer**: Use existing 4-byte header pattern
3. **Flash Management**:
   - Erase flash region command
   - Write flash command with address/data
   - Verify checksum command
4. **Status Monitoring**:
   - Query transfer progress
   - Get remaining space
   - Query write status

## Key Source Files

- `right/src/usb_protocol_handler.c` and `.h` - Main dispatcher
- `right/src/usb_commands/` - Individual command handlers
- `right/src/usb_interfaces/usb_interface_generic_hid.c` - HID interface
- `shared/buffer.h` - Buffer utilities
- `lib/agent/packages/uhk-usb/src/uhk-operations.ts` - Agent operations

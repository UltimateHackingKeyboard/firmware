# Configuration Buffer APIs

This document describes the user configuration buffer system that could potentially be used for firmware image storage during module flashing.

## Buffer Architecture

The firmware uses a **three-buffer system** for configuration management:

```c
typedef enum {
    ConfigBufferId_HardwareConfig,
    ConfigBufferId_StagingUserConfig,
    ConfigBufferId_ValidatedUserConfig,
} config_buffer_id_t;
```

### Memory Sizes

| Platform | Hardware Config | User Config |
|----------|----------------|-------------|
| UHK60    | 64 bytes       | 32,704 bytes |
| UHK80    | 64 bytes       | 32,768 bytes |

**Note**: Module firmware images are typically 40-60 KB for UHK60 modules, which exceeds the current user config buffer capacity. Alternative storage strategies may be needed.

## Core Data Structures

### Buffer Structure

From `right/src/config_parser/basic_types.h`:

```c
typedef struct {
    bool isValid;       // Flag indicating if buffer contains valid configuration
    uint8_t *buffer;    // Pointer to raw byte buffer
    uint16_t offset;    // Current read/write position for parsing
} config_buffer_t;
```

### Buffer Instances

From `right/src/config_parser/config_globals.c`:

```c
config_buffer_t HardwareConfigBuffer;
config_buffer_t StagingUserConfigBuffer;
config_buffer_t ValidatedUserConfigBuffer;
```

## Key Functions

### Buffer Access

```c
// Get buffer descriptor from ID
config_buffer_t* ConfigBufferIdToConfigBuffer(config_buffer_id_t configBufferId);

// Get buffer size
uint16_t ConfigBufferIdToBufferSize(config_buffer_id_t configBufferId);

// Validate buffer ID
bool IsConfigBufferIdValid(config_buffer_id_t configBufferId);
```

### Parsing Primitives

From `right/src/config_parser/basic_types.h`:

```c
uint8_t ReadUInt8(config_buffer_t *buffer);
uint16_t ReadUInt16(config_buffer_t *buffer);
uint32_t ReadUInt32(config_buffer_t *buffer);
```

## USB Read/Write APIs

### Read Config Command

**File**: `right/src/usb_commands/usb_command_read_config.c`

**Protocol**:
- Byte 0: Command (0x04)
- Byte 1: Config buffer ID
- Byte 2: Length to read (max 62 bytes)
- Bytes 3-4: Offset (little-endian uint16)
- Response: [Status(1)] [Data(length)]

### Write Config Command

**File**: `right/src/usb_commands/usb_command_write_config.c`

**Protocol**:
- Byte 0: Command (0x05 or 0x06)
- Byte 1: Length to write
- Bytes 2-3: Offset (little-endian uint16)
- Bytes 4+: Data to write (max 59 bytes per packet)

## Storage Operations

### UHK60 - EEPROM

**File**: `right/src/eeprom.c`

```c
status_t EEPROM_LaunchTransfer(storage_operation_t operation,
                               config_buffer_id_t configBufferId,
                               void (*successCallback));
```

- I2C-based EEPROM at 100 kHz (I2C1 at 1 MHz)
- 64-byte page writes
- Sequential page writes with callbacks

### UHK80 - Flash

**File**: `device/src/flash.c`

```c
uint8_t Flash_LaunchTransfer(storage_operation_t operation,
                             config_buffer_id_t configBufferId,
                             void (*successCallback));
```

- Flash area API with 4K alignment
- Erase before write required

## Implications for Firmware Storage

Current configuration buffer capacity (~32 KB) is insufficient for typical module firmware images (~40-60 KB). Options include:

1. **Streaming approach**: Transfer firmware in chunks directly to module without storing in RAM
2. **External storage**: Use separate flash region on UHK80
3. **Multiple transfers**: Split firmware and transfer in multiple sessions

## Key Source Files

- `right/src/config_manager.h` - Runtime config struct
- `right/src/config_parser/config_globals.h` - Buffer types & IDs
- `right/src/config_parser/basic_types.h` - Parsing primitives
- `right/src/eeprom.c` - UHK60 storage
- `device/src/flash.c` - UHK80 storage

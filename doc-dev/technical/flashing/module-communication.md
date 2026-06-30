# Module Communication

This document describes how the right half communicates with modules (key cluster, trackball, trackpad) over I2C and UART.

## I2C Addresses

From `shared/i2c_addresses.h`:

| Module | Firmware Address | Bootloader Address |
|--------|-----------------|-------------------|
| Left Half | 0x08 | 0x10 |
| Left Module | 0x18 | 0x20 |
| Right Module | 0x28 | 0x30 |
| Touchpad | 0x2D | 0x6D |

## I2C API (UHK60)

**File**: `right/src/i2c.c`

```c
// Non-blocking I2C operations
I2cAsyncWrite(uint8_t i2cAddress, uint8_t *data, size_t dataSize);
I2cAsyncRead(uint8_t i2cAddress, uint8_t *data, size_t dataSize);

// Message-based operations with CRC16
I2cAsyncWriteMessage(uint8_t i2cAddress, i2c_message_t *message);
I2cAsyncReadMessage(uint8_t i2cAddress, i2c_message_t *message);
```

**Hardware Configuration**:
- Main bus: I2C0 at 100 kHz (30 kHz for bootloader)
- EEPROM bus: I2C1 at 1 MHz

## UART API (UHK80)

**File**: `device/src/keyboard/uart_modules.c`

- Runs in separate kernel thread
- Semaphore-based synchronization
- Configurable timeout (MODULE_CONNECTION_TIMEOUT)

## Slave Scheduler

**File**: `right/src/slave_scheduler.c`

The slave scheduler is a **round-robin, interrupt-driven I2C master** managing all module communication.

### Slave Types

```c
typedef enum {
    SlaveId_LeftKeyboardHalf,
    SlaveId_LeftModule,
    SlaveId_RightModule,
    SlaveId_RightTouchpad,
    SlaveId_RightLedDriver,
    SlaveId_LeftLedDriver,
    SlaveId_ModuleLeftLedDriver,
    SlaveId_KbootDriver,
} slave_id_t;
```

### Slave Data Structure

```c
typedef struct {
    uint8_t perDriverId;
    slave_init_t *init;
    slave_update_t *update;
    slave_disconnect_t *disconnect;
    slave_connect_t *connect;
    bool isConnected;
    status_t previousStatus;
} uhk_slave_t;

extern uhk_slave_t Slaves[SLAVE_COUNT];  // 8 slaves
```

### Scheduling Flow

1. `slaveSchedulerCallback()` - I2C interrupt handler
2. Finalize previous transfer, check for errors
3. Update connection status (connect/disconnect callbacks)
4. Try next slave(s) in round-robin order
5. Call `slave->update()` to get next transfer
6. Issue scheduled I2C transfer

## Module Discovery Protocol

**File**: `right/src/slave_drivers/uhk_module_driver.c`

Discovery phases executed in sequence:

1. **Sync Request**: Validate "SYNC" string
2. **Protocol Version**: Get 3-byte version
3. **Firmware Version**: Get 3-byte version
4. **Module ID**: Identify module type
5. **Key Count**: Number of keys
6. **Pointer Count**: Number of pointer inputs
7. **Git Tag**: Version tag (protocol v4.2+)
8. **Git Repo**: Repository name
9. **Firmware Checksum**: MD5 checksum
10. **Update Loop**: Periodic key state polling

### Module State Structure

```c
typedef struct {
    uint8_t moduleId;
    version_t moduleProtocolVersion;
    version_t firmwareVersion;
    uhk_module_phase_t phase;
    i2c_message_t rxMessage;
    uint8_t firmwareI2cAddress;
    uint8_t bootloaderI2cAddress;
    uint8_t keyCount;
    uint8_t pointerCount;
    char gitTag[MAX_STRING_PROPERTY];
    char firmwareChecksum[MD5_CHECKSUM];
} uhk_module_state_t;
```

## Slave Protocol

**File**: `shared/slave_protocol.h`

### Message Format

```c
typedef struct {
    uint8_t length;
    uint16_t crc;
    uint8_t data[SLAVE_PROTOCOL_MAX_PAYLOAD];  // max 64 bytes
    uint8_t padding;
} ATTR_PACKED i2c_message_t;
```

### Commands

```c
typedef enum {
    SlaveCommand_RequestProperty = 0,
    SlaveCommand_JumpToBootloader = 1,
    SlaveCommand_RequestKeyStates = 2,
    SlaveCommand_SetTestLed = 3,
    SlaveCommand_SetLedPwmBrightness = 4,
    SlaveCommand_ModuleSpecificCommand = 5,
} slave_command_t;
```

### Properties

```c
typedef enum {
    SlaveProperty_Sync = 0,
    SlaveProperty_ModuleProtocolVersion = 1,
    SlaveProperty_FirmwareVersion = 2,
    SlaveProperty_ModuleId = 3,
    SlaveProperty_KeyCount = 4,
    SlaveProperty_PointerCount = 5,
    SlaveProperty_GitTag = 6,
    SlaveProperty_GitRepo = 7,
    SlaveProperty_FirmwareChecksum = 8,
} slave_property_t;
```

## Module Reset Mechanisms

### Jump to Bootloader

```c
case UhkModulePhase_JumpToBootloader:
    txMessage.data[0] = SlaveCommand_JumpToBootloader;
    txMessage.length = 1;
    res.status = tx(i2cAddress);
    break;
```

### Trackpoint Reset

```c
void UhkModuleSlaveDriver_SendTrackpointCommand(module_specific_command_t command);
```

Commands: `ResetTrackpoint`, `RunTrackpoint`, `TrackpointSignalData`, `TrackpointSignalClock`

## Connection Status Management

```c
void UhkModuleSlaveDriver_Disconnect(uint8_t uhkModuleDriverId) {
    // Clear module state
    // Clear key states
    // Schedule reconnection timeout (350ms)
}
```

## Communication Flow Diagram

```
Right Half                          Module (I2C)
    |                                   |
    |-- SlaveCommand_RequestProperty -->|
    |      (SlaveProperty_Sync)         |
    |<--------- "SYNC" response --------|
    |                                   |
    |-- SlaveCommand_RequestProperty -->|
    |   (SlaveProperty_ModuleId)        |
    |<--------- ModuleId (1 byte) ------|
    |                                   |
    |-- [Loop: RequestKeyStates] ------>|
    |<--------- KeyStates + Pointer ----|
```

## Key Source Files

- `right/src/slave_scheduler.c` and `.h` - Round-robin scheduler
- `right/src/slave_drivers/uhk_module_driver.c` - Module discovery/polling
- `right/src/slave_drivers/kboot_driver.c` - Bootloader communication
- `right/src/i2c.c` - I2C primitives (UHK60)
- `device/src/keyboard/uart_modules.c` - UART modules (UHK80)
- `shared/slave_protocol.h` - Protocol definitions
- `shared/i2c_addresses.h` - I2C address mapping

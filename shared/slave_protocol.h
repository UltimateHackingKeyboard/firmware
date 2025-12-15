#ifndef __SLAVE_PROTOCOL_H__
#define __SLAVE_PROTOCOL_H__

// Includes:

    #include <stdint.h>
    #include "attributes.h"

// Macros:

    #define SLAVE_PROTOCOL_OVER_UART (DEVICE_IS_UHK80_RIGHT && (PinWiring_ActualUartDebugMode != UartDebugMode_I2CMode && PinWiring_ActualUartDebugMode != UartDebugMode_DebugOverModules))

    #define I2C_MESSAGE_HEADER_LENGTH 3
    #define I2C_MESSAGE_MAX_PAYLOAD_LENGTH 64
    #define I2C_MESSAGE_MAX_TOTAL_LENGTH (I2C_MESSAGE_HEADER_LENGTH + I2C_MESSAGE_MAX_PAYLOAD_LENGTH)

    #define SLAVE_SYNC_STRING "SYNC"
    #define SLAVE_SYNC_STRING_LENGTH (sizeof(SLAVE_SYNC_STRING) - 1)

// Typedefs:

    typedef enum {
        SlaveCommand_RequestProperty,
        SlaveCommand_JumpToBootloader,
        SlaveCommand_RequestKeyStates,
        SlaveCommand_SetTestLed,
        SlaveCommand_SetLedPwmBrightness,
        SlaveCommand_ModuleSpecificCommand,
    } slave_command_t;

    typedef enum {
        ModuleSpecificCommand_ResetTrackpoint,
        ModuleSpecificCommand_RunTrackpoint,
        ModuleSpecificCommand_TrackpointSignalData,
        ModuleSpecificCommand_TrackpointSignalClock,
    } module_specific_command_t;

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

    typedef enum {
        ModuleId_Unavailable = 0,
        ModuleId_RightKeyboardHalf = 0, // Always connected
        ModuleId_LeftKeyboardHalf = 1,
        ModuleId_KeyClusterLeft = 2,
        ModuleId_FirstRealModule = ModuleId_KeyClusterLeft,
        ModuleId_TrackballRight = 3,
        ModuleId_TrackpointRight = 4,
        ModuleId_TouchpadRight = 5,
        ModuleId_Next = 6,
        ModuleId_FirstModule = ModuleId_KeyClusterLeft,
        ModuleId_Last = ModuleId_Next,
        ModuleId_RealModuleCount = ModuleId_Last - ModuleId_FirstRealModule + 1,
        ModuleId_AllModuleCount = ModuleId_Last + 1,
        ModuleId_Dongle = 254,
        ModuleId_Invalid = 255,
    } module_id_t;

    typedef struct {
        uint8_t length;
        uint16_t crc;
        uint8_t data[I2C_MESSAGE_MAX_PAYLOAD_LENGTH];
    } ATTR_PACKED i2c_message_t;

    typedef struct {
    } ATTR_PACKED pointer_debug_info_t;

    typedef struct {
        int16_t x;
        int16_t y;
        pointer_debug_info_t debugInfo;
    } ATTR_PACKED pointer_delta_t;

// Variables:

    extern char SlaveSyncString[];

#endif

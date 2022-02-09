#ifndef __SLAVE_PROTOCOL_H__
#define __SLAVE_PROTOCOL_H__

// Includes:

    #include "fsl_common.h"
    #include "attributes.h"

// Macros:

    #define I2C_MESSAGE_HEADER_LENGTH 3
    #define I2C_MESSAGE_MAX_PAYLOAD_LENGTH 255
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
    } slave_command_t;

    typedef enum {
        SlaveProperty_Sync,
        SlaveProperty_ModuleProtocolVersion,
        SlaveProperty_FirmwareVersion,
        SlaveProperty_ModuleId,
        SlaveProperty_KeyCount,
        SlaveProperty_PointerCount,
        SlaveProperty_GitTag,
        SlaveProperty_GitRepo,
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
        ModuleId_ModuleCount = ModuleId_Last - ModuleId_FirstRealModule + 1,
        ModuleId_AllCount = ModuleId_Last + 1,
    } module_id_t;

    typedef struct {
        uint8_t length;
        uint16_t crc;
        uint8_t data[I2C_MESSAGE_MAX_PAYLOAD_LENGTH];
    } ATTR_PACKED i2c_message_t;

    typedef struct {
        int16_t x;
        int16_t y;
    } ATTR_PACKED pointer_delta_t;

// Variables:

    extern char SlaveSyncString[];

#endif

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
        SlaveCommand_RequestKeyStates,
        SlaveCommand_JumpToBootloader,
        SlaveCommand_SetTestLed,
        SlaveCommand_SetLedPwmBrightness,
    } slave_command_t;

    typedef enum {
        SlaveProperty_Sync,
        SlaveProperty_ProtocolVersion,
        SlaveProperty_ModuleId,
        SlaveProperty_Features,
    } slave_property_t;

    typedef enum {
        ModuleId_LeftKeyboardHalf = 1,
        ModuleId_KeyClusterLeft   = 2,
        ModuleId_TrackballRight   = 3,
        ModuleId_TrackpointRight  = 4,
        ModuleId_TouchpadRight    = 5,
    } module_id_t;

    typedef struct {
        uint8_t keyCount;
        bool hasPointer;
    } uhk_module_features_t;

    typedef struct {
        uint8_t length;
        uint16_t crc;
        uint8_t data[I2C_MESSAGE_MAX_PAYLOAD_LENGTH];
    } ATTR_PACKED i2c_message_t;

// Variables:

    extern char SlaveSyncString[];

#endif

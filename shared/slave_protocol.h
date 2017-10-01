#ifndef __SLAVE_PROTOCOL_H__
#define __SLAVE_PROTOCOL_H__

// Macros:

    #define I2C_MESSAGE_HEADER_LENGTH 3
    #define I2C_MESSAGE_MAX_PAYLOAD_LENGTH 255
    #define I2C_MESSAGE_MAX_TOTAL_LENGTH (I2C_MESSAGE_HEADER_LENGTH + I2C_MESSAGE_MAX_PAYLOAD_LENGTH)

// Typedefs:

    typedef enum {
        SlaveCommand_RequestProperty,
        SlaveCommand_RequestKeyStates,
        SlaveCommand_JumpToBootloader,
        SlaveCommand_SetTestLed,
        SlaveCommand_SetLedPwmBrightness,
    } slave_command_t;

    typedef enum {
        SlaveProperty_Features,
    } slave_property_t;

    typedef struct {
        uint8_t keyCount;
        bool hasPointer;
    } uhk_module_features_t;

    typedef struct {
        uint8_t length;
        uint16_t crc;
        uint8_t data[I2C_MESSAGE_MAX_PAYLOAD_LENGTH];
    } __attribute__ ((packed)) i2c_message_t;

#endif

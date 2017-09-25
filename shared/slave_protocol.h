#ifndef __SLAVE_PROTOCOL_H__
#define __SLAVE_PROTOCOL_H__

// Macros:

    #define I2C_MESSAGE_MAX_LENGTH 255
    #define I2C_BUFFER_MAX_LENGTH (I2C_MESSAGE_MAX_LENGTH + 3)

// Typedefs:

    typedef enum {
        SlaveCommand_RequestKeyStates,
        SlaveCommand_SetTestLed,
        SlaveCommand_SetLedPwmBrightness,
        SlaveCommand_JumpToBootloader,
    } slave_command_t;

    typedef struct {
        uint8_t length;
        uint16_t crc;
        uint8_t data[I2C_MESSAGE_MAX_LENGTH];
    } __attribute__ ((packed)) i2c_message_t;

#endif

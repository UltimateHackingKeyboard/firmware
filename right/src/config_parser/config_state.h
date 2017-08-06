#ifndef __CONFIG_STATE_H__
#define __CONFIG_STATE_H__

// Includes:

    #include <stdint.h>
    #include "fsl_common.h"
    #include "eeprom.h"

// Macros:

    #define HARDWARE_CONFIG_SIZE 64
    #define USER_CONFIG_SIZE (EEPROM_SIZE - HARDWARE_CONFIG_SIZE)

// Typedefs:

    typedef struct {
        uint8_t *buffer;
        uint16_t offset;
    } config_buffer_t;

// Variables:

    extern config_buffer_t HardwareConfigBuffer;
    extern config_buffer_t UserConfigBuffer;

// Functions:

    uint8_t readUInt8(config_buffer_t *buffer);
    uint16_t readUInt16(config_buffer_t *buffer);
    int16_t readInt16(config_buffer_t *buffer);
    bool readBool(config_buffer_t *buffer);
    uint16_t readCompactLength(config_buffer_t *buffer);
    const char *readString(config_buffer_t *buffer, uint16_t *len);

#endif

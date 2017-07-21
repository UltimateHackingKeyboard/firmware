#ifndef __CONFIG_STATE_H__
#define __CONFIG_STATE_H__

// Includes:

    #include <stdint.h>
    #include "fsl_common.h"

// Macros:

    #define EEPROM_SIZE (32*1024)

// Typedefs:

    typedef struct {
        uint8_t buffer[EEPROM_SIZE];
        uint16_t offset;
    } config_buffer_t;

// Variables:

    extern config_buffer_t ConfigBuffer;

// Functions:

    uint8_t readUInt8(config_buffer_t *buffer);
    uint16_t readUInt16(config_buffer_t *buffer);
    int16_t readInt16(config_buffer_t *buffer);
    bool readBool(config_buffer_t *buffer);
    uint16_t readCompactLength(config_buffer_t *buffer);
    const char *readString(config_buffer_t *buffer, uint16_t *len);

#endif

#ifndef __CONFIG_STATE_H__
#define __CONFIG_STATE_H__

// Includes:

    #include <stdint.h>
    #include "fsl_common.h"

// Macros:

    #define EEPROM_SIZE (32*1024)

// Typedefs:

    typedef struct {
        uint8_t *const buffer;
        uint16_t offset;
    } serialized_buffer_t;

// Variables:

    extern serialized_buffer_t ConfigBuffer;

// Functions:

    uint8_t readUInt8(serialized_buffer_t *buffer);
    uint16_t readUInt16(serialized_buffer_t *buffer);
    bool readBool(serialized_buffer_t *buffer);
    uint16_t readCompactLength(serialized_buffer_t *buffer);
    const char *readString(serialized_buffer_t *buffer, uint16_t *len);

#endif

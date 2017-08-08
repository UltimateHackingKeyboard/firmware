#ifndef __BASIC_TYPES_H__
#define __BASIC_TYPES_H__

// Includes:

    #include "fsl_common.h"

// Typedefs:

    typedef struct {
        uint8_t *buffer;
        uint16_t offset;
    } config_buffer_t;

// Functions:

    uint8_t readUInt8(config_buffer_t *buffer);
    uint16_t readUInt16(config_buffer_t *buffer);
    int16_t readInt16(config_buffer_t *buffer);
    bool readBool(config_buffer_t *buffer);
    uint16_t readCompactLength(config_buffer_t *buffer);
    const char *readString(config_buffer_t *buffer, uint16_t *len);

#endif

#ifndef __BASIC_TYPES_H__
#define __BASIC_TYPES_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
#ifndef __ZEPHYR__
    #include "fsl_common.h"
#endif

// Typedefs:

    typedef struct {
        uint8_t *buffer;
        uint16_t offset;
    } config_buffer_t;

// Functions:

    uint8_t ReadUInt8(config_buffer_t *buffer);
    uint16_t ReadUInt16(config_buffer_t *buffer);
    int16_t ReadInt16(config_buffer_t *buffer);
    uint32_t ReadUInt32(config_buffer_t *buffer);
    float ReadFloat(config_buffer_t *buffer);
    bool ReadBool(config_buffer_t *buffer);
    uint16_t ReadCompactLength(config_buffer_t *buffer);
    const char *ReadString(config_buffer_t *buffer, uint16_t *len);

#endif

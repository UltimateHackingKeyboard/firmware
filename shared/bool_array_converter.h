#ifndef __BOOL_ARRAY_CONVERTER_H__
#define __BOOL_ARRAY_CONVERTER_H__

// Includes:

    #include <stdint.h>

// Macros:

    #define BOOL_BYTES_TO_BITS_COUNT(BYTE_COUNT) (BYTE_COUNT/8 + (BYTE_COUNT % 8 ? 1 : 0))

// Functions:

    void BoolBytesToBits(uint8_t *srcBytes, uint8_t *dstBits, uint8_t byteCount);
    void BoolBitsToBytes(uint8_t *srcBits, uint8_t *dstBytes, uint8_t byteCount);

#endif

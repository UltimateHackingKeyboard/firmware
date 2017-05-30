#ifndef __BOOL_ARRAY_CONVERTER_H__
#define __BOOL_ARRAY_CONVERTER_H__

// Includes:

    #include <stdint.h>

// Functions:

    void BoolBytesToBits(uint8_t *srcBytes, uint8_t *dstBits, uint8_t byteCount);
    void BoolBitsToBytes(uint8_t *srcBits, uint8_t *dstBytes, uint8_t byteCount);

#endif

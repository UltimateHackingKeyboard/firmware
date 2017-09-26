#include <string.h>
#include "bool_array_converter.h"

void BoolBytesToBits(uint8_t *srcBytes, uint8_t *dstBits, uint8_t byteCount)
{
    memset(dstBits, 0, byteCount/8 + (byteCount % 8 ? 1 : 0));
    for (uint8_t i=0; i<byteCount; i++) {
        dstBits[i/8] |= !!srcBytes[i] << (i % 8);
    }
}

void BoolBitsToBytes(uint8_t *srcBits, uint8_t *dstBytes, uint8_t byteCount)
{
    for (uint8_t i=0; i<byteCount; i++) {
        dstBytes[i] = !!(srcBits[i/8] & (1 << (i % 8)));
    }
}

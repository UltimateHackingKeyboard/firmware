#include <string.h>
#include <stdbool.h>
#include "bool_array_converter.h"

void BoolBytesToBits(const uint8_t *srcBytes, uint8_t *dstBits, uint8_t byteCount)
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

void BoolBitToBytes(bool srcVal, uint8_t srcPos, uint8_t *dstBytes)
{
    uint8_t bitMask = 1 << (srcPos % 8);
    uint8_t resultBit = (!!srcVal) << (srcPos % 8);
    uint8_t resultByte = (dstBytes[srcPos/8] & (~bitMask)) | resultBit;
    dstBytes[srcPos/8] = resultByte;
}

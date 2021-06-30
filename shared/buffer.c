#include "buffer.h"

uint8_t GetBufferUint8(const uint8_t *buffer, uint32_t offset)
{
    return *(const uint8_t*)(buffer + offset);
}

uint16_t GetBufferUint16(const uint8_t *buffer, uint32_t offset)
{
    return *(const uint16_t*)(buffer+ offset);
}

uint32_t GetBufferUint32(const uint8_t *buffer, uint32_t offset)
{
    return *(const uint32_t*)(buffer + offset);
}

uint8_t  GetBufferUint8Be(const uint8_t *buffer, uint32_t offset)
{
    return buffer[offset];
}

uint16_t GetBufferUint16Be(const uint8_t *buffer, uint32_t offset)
{
    return (buffer[offset] << 8) + (buffer[offset+1] << 0);
}

uint32_t GetBufferUint32Be(const uint8_t *buffer, uint32_t offset)
{
    return (buffer[offset] << 24) + (buffer[offset+1] << 16) + (buffer[offset+2] << 8) + (buffer[offset+3] << 0);
}

void SetBufferUint8(uint8_t *buffer, uint32_t offset, uint8_t value)
{
    *(uint8_t*)(buffer + offset) = value;
}

void SetBufferUint16(uint8_t *buffer, uint32_t offset, uint16_t value)
{
    *(uint16_t*)(buffer + offset) = value;
}

void SetBufferUint32(uint8_t *buffer, uint32_t offset, uint32_t value)
{
    *(uint32_t*)(buffer + offset) = value;
}

void SetBufferInt8(uint8_t *buffer, uint32_t offset, int8_t value)
{
    *(int8_t*)(buffer + offset) = value;
}

void SetBufferInt16(uint8_t *buffer, uint32_t offset, int16_t value)
{
    *(int16_t*)(buffer + offset) = value;
}

void SetBufferInt32(uint8_t *buffer, uint32_t offset, int32_t value)
{
    *(int32_t*)(buffer + offset) = value;
}

void SetBufferUint8Be(uint8_t *buffer, uint32_t offset, uint8_t value)
{
    buffer[offset] = value;
}

void SetBufferUint16Be(uint8_t *buffer, uint32_t offset, uint16_t value)
{
    buffer[offset+0] = (value >> 8) & 0xff;
    buffer[offset+1] = (value >> 0) & 0xff;
}

void SetBufferUint32Be(uint8_t *buffer, uint32_t offset, uint32_t value)
{
    buffer[offset+0] = (value >> 24) & 0xff;
    buffer[offset+1] = (value >> 16) & 0xff;
    buffer[offset+2] = (value >>  8) & 0xff;
    buffer[offset+3] = (value >>  0) & 0xff;
}

void SetBufferFloat(uint8_t *buffer, uint32_t offset, float value)
{
    *(float*)(buffer + offset) = value;
}

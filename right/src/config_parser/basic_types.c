#include "basic_types.h"

uint8_t ReadUInt8(config_buffer_t *buffer)
{
    return buffer->buffer[buffer->offset++];
}

uint16_t ReadUInt16(config_buffer_t *buffer)
{
    uint16_t uInt16 = ReadUInt8(buffer);
    uInt16 |= ReadUInt8(buffer) << 8;
    return uInt16;
}

int16_t ReadInt16(config_buffer_t *buffer)
{
    return ReadUInt16(buffer);
}

bool ReadBool(config_buffer_t *buffer)
{
    return ReadUInt8(buffer);
}

uint16_t ReadCompactLength(config_buffer_t *buffer)
{
    uint16_t compactLength = ReadUInt8(buffer);
    return compactLength == 0xFF ? ReadUInt16(buffer) : compactLength;
}

const char *ReadString(config_buffer_t *buffer, uint16_t *len)
{
    const char *string;
    *len = ReadCompactLength(buffer);
    string = (const char *)(buffer->buffer + buffer->offset);
    buffer->offset += *len;
    return string;
}

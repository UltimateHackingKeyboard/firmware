#include "basic_types.h"

uint8_t readUInt8(config_buffer_t *buffer) {
    return buffer->buffer[buffer->offset++];
}

uint16_t readUInt16(config_buffer_t *buffer) {
    uint16_t uInt16 = readUInt8(buffer);

    uInt16 |= readUInt8(buffer) << 8;
    return uInt16;
}

int16_t readInt16(config_buffer_t *buffer) {
    return readUInt16(buffer);
}

bool readBool(config_buffer_t *buffer) {
    return readUInt8(buffer);
}

uint16_t readCompactLength(config_buffer_t *buffer) {
    uint16_t compactLength = readUInt8(buffer);

    return compactLength == 0xFF ? readUInt16(buffer) : compactLength;
}

const char *readString(config_buffer_t *buffer, uint16_t *len) {
    const char *string;

    *len = readCompactLength(buffer);
    string = (const char *)(buffer->buffer + buffer->offset);
    buffer->offset += *len;
    return string;
}

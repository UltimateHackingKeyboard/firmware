#include "config_state.h"

config_buffer_t ConfigBuffer;

uint8_t readUInt8(config_buffer_t *buffer) {
    return buffer->buffer[buffer->offset++];
}

uint16_t readUInt16(config_buffer_t *buffer) {
    uint16_t uInt16 = *(uint16_t *)(buffer->buffer + buffer->offset);

    buffer->offset += 2;
    return uInt16;
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
    string = buffer->buffer + buffer->offset;
    buffer->offset += *len;
    return string;
}

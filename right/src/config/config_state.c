#include "config_state.h"

static uint8_t config[EEPROM_SIZE];
serialized_buffer_t ConfigBuffer = { config };

uint8_t readUInt8(serialized_buffer_t *buffer) {
    return buffer->buffer[buffer->offset++];
}

uint16_t readUInt16(serialized_buffer_t *buffer) {
    uint16_t uInt16 = *(uint16_t *)(buffer->buffer + buffer->offset);

    buffer->offset += 2;
    return uInt16;
}

bool readBool(serialized_buffer_t *buffer) {
    return readUInt8(buffer);
}

uint16_t readCompactLength(serialized_buffer_t *buffer) {
    uint16_t compactLength = readUInt8(buffer);

    return compactLength == 0xFF ? readUInt16(buffer) : compactLength;
}

const char *readString(serialized_buffer_t *buffer, uint16_t *len) {
    const char *string;

    *len = readCompactLength(buffer);
    string = buffer->buffer + buffer->offset;
    buffer->offset += *len;
    return string;
}

#include "config_state.h"

static uint8_t config[EEPROM_SIZE];
serialized_buffer_t ConfigBuffer = { config };

uint8_t readUInt8(serialized_buffer_t *buffer) {
    return buffer->buffer[buffer->offset++];
}

uint16_t readUInt16(serialized_buffer_t *buffer) {
    uint8_t firstByte = buffer->buffer[buffer->offset++];
    return firstByte + (buffer->buffer[buffer->offset++] << 8);
}

bool readBool(serialized_buffer_t *buffer) {
    return buffer->buffer[buffer->offset++] == 1;
}

uint16_t readCompactLength(serialized_buffer_t *buffer) {
    uint16_t length = readUInt8(buffer);
    if (length == 0xFF) {
        length = readUInt16(buffer);
    }
    return length;
}

const char *readString(serialized_buffer_t *buffer, uint16_t *len) {
    const char *str = (const char *)&(buffer->buffer[buffer->offset]);

    *len = readCompactLength(buffer);
    buffer->offset += *len;

    return str;
}

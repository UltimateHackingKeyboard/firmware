#include "parse_config.h"
#include "parse_keymap.h"

static parser_error_t parseModuleConfiguration(serialized_buffer_t *buffer) {
    uint8_t id = readUInt8(buffer);
    uint8_t initialPointerSpeed = readUInt8(buffer);
    uint8_t pointerAcceleration = readUInt8(buffer);
    uint8_t maxPointerSpeed = readUInt8(buffer);

    (void)id;
    (void)initialPointerSpeed;
    (void)pointerAcceleration;
    (void)maxPointerSpeed;
    return ParserError_Success;
}

parser_error_t ParseConfig(serialized_buffer_t *buffer) {
    uint16_t dataModelVersion = readUInt16(buffer);
    parser_error_t errorCode;
    uint16_t moduleConfigurationCount = readCompactLength(buffer);
    uint16_t macroCount;
    uint16_t keymapCount;

    (void)dataModelVersion;
    for (uint8_t moduleConfigurationIdx = 0; moduleConfigurationIdx < moduleConfigurationCount; moduleConfigurationIdx++) {
        errorCode = parseModuleConfiguration(buffer);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
    }
    macroCount = readCompactLength(buffer);
    for (uint8_t macroIdx = 0; macroIdx < macroCount; macroIdx++) {
        // errorCode = ParseMacro(buffer);
        // if (errorCode != ParserError_Success) {
        //     return errorCode;
        // }
    }
    keymapCount = readCompactLength(buffer);
    for (uint8_t keymapIdx = 0; keymapIdx < keymapCount; keymapIdx++) {
        errorCode = ParseKeymap(buffer);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
    }
    return ParserError_Success;
}

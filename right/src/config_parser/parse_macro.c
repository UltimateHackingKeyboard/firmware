#include "parse_macro.h"
#include "config_globals.h"
#include "macros.h"

parser_error_t parseKeyMacroAction(config_buffer_t *buffer, serialized_macro_action_type_t macroActionType) {
    uint8_t keyMacroType = macroActionType - SerializedMacroActionType_KeyMacroAction;
    uint8_t action = keyMacroType & 0b11;
    uint8_t type;
    uint8_t scancode;
    uint8_t modifierMask;

    keyMacroType >>= 2;
    type = keyMacroType & 0b11;
    keyMacroType >>= 2;
    if (keyMacroType & 0b10) {
        scancode = readUInt8(buffer);
    }
    if (keyMacroType & 0b01) {
        modifierMask = readUInt8(buffer);
    }
    (void)action;
    (void)type;
    (void)scancode;
    (void)modifierMask;
    return ParserError_Success;
}

parser_error_t parseMouseButtonMacroAction(config_buffer_t *buffer, serialized_macro_action_type_t macroActionType) {
    uint8_t action = macroActionType - SerializedMacroActionType_MouseButtonMacroAction;
    uint8_t mouseButtonsMask = readUInt8(buffer);

    (void)action;
    (void)mouseButtonsMask;
    return ParserError_Success;
}

parser_error_t parseMoveMouseMacroAction(config_buffer_t *buffer) {
    int16_t x = readInt16(buffer);
    int16_t y = readInt16(buffer);

    (void)x;
    (void)y;
    return ParserError_Success;
}

parser_error_t parseScrollMouseMacroAction(config_buffer_t *buffer) {
    int16_t x = readInt16(buffer);
    int16_t y = readInt16(buffer);

    (void)x;
    (void)y;
    return ParserError_Success;
}

parser_error_t parseDelayMacroAction(config_buffer_t *buffer) {
    int16_t delay = readInt16(buffer);

    (void)delay;
    return ParserError_Success;
}

parser_error_t parseTextMacroAction(config_buffer_t *buffer) {
    uint16_t textLen;
    const char *text = readString(buffer, &textLen);

    (void)text;
    return ParserError_Success;
}

parser_error_t parseMacroAction(config_buffer_t *buffer) {
    uint8_t macroActionType = readUInt8(buffer);

    switch (macroActionType) {
        case SerializedMacroActionType_KeyMacroAction ... SerializedMacroActionType_LastKeyMacroAction:
            return parseKeyMacroAction(buffer, macroActionType);
        case SerializedMacroActionType_MouseButtonMacroAction ... SerializedMacroActionType_LastMouseButtonMacroAction:
            return parseMouseButtonMacroAction(buffer, macroActionType);
        case SerializedMacroActionType_MoveMouseMacroAction:
            return parseMoveMouseMacroAction(buffer);
        case SerializedMacroActionType_ScrollMouseMacroAction:
            return parseScrollMouseMacroAction(buffer);
        case SerializedMacroActionType_DelayMacroAction:
            return parseDelayMacroAction(buffer);
        case SerializedMacroActionType_TextMacroAction:
            return parseTextMacroAction(buffer);
    }
    return ParserError_InvalidSerializedMacroActionType;
}

parser_error_t ParseMacro(config_buffer_t *buffer, uint8_t macroIdx) {
    uint16_t offset = buffer->offset;
    parser_error_t errorCode;
    uint16_t nameLen;
    bool isLooped = readBool(buffer);
    bool isPrivate = readBool(buffer);
    const char *name = readString(buffer, &nameLen);
    uint16_t macroActionsCount = readCompactLength(buffer);

    (void)isLooped;
    (void)isPrivate;
    (void)name;
    if (!ParserRunDry) {
        AllMacros[macroIdx].offset = offset;
        AllMacros[macroIdx].macroActionsCount = macroActionsCount;
    }
    for (uint16_t i = 0; i < macroActionsCount; i++) {
        errorCode = parseMacroAction(buffer);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
    }
    return ParserError_Success;
}

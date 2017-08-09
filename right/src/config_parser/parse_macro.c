#include "parse_macro.h"
#include "config_globals.h"
#include "macros.h"

parser_error_t parseKeyMacroAction(config_buffer_t *buffer, macro_action_t *macroAction, serialized_macro_action_type_t macroActionType)
{
    uint8_t keyMacroType = macroActionType - SerializedMacroActionType_KeyMacroAction;
    uint8_t action = keyMacroType & 0b11;
    uint8_t type;
    uint8_t scancode;
    uint8_t modifierMask;

    keyMacroType >>= 2;
    type = keyMacroType & 0b11;
    keyMacroType >>= 2;
    scancode = keyMacroType & 0b10 ? readUInt8(buffer) : 0;
    modifierMask = keyMacroType & 0b01 ? readUInt8(buffer) : 0;
    macroAction->type = MacroActionType_Key;
    macroAction->key.action = action;
    macroAction->key.type = type;
    macroAction->key.scancode = scancode;
    macroAction->key.modifierMask = modifierMask;
    return ParserError_Success;
}

parser_error_t parseMouseButtonMacroAction(config_buffer_t *buffer, macro_action_t *macroAction, serialized_macro_action_type_t macroActionType)
{
    uint8_t action = macroActionType - SerializedMacroActionType_MouseButtonMacroAction;
    uint8_t mouseButtonsMask = readUInt8(buffer);

    macroAction->type = MacroActionType_MouseButton;
    macroAction->mouseButton.action = action;
    macroAction->mouseButton.mouseButtonsMask = mouseButtonsMask;
    return ParserError_Success;
}

parser_error_t parseMoveMouseMacroAction(config_buffer_t *buffer, macro_action_t *macroAction)
{
    int16_t x = readInt16(buffer);
    int16_t y = readInt16(buffer);

    macroAction->type = MacroActionType_MoveMouse;
    macroAction->moveMouse.x = x;
    macroAction->moveMouse.y = y;
    return ParserError_Success;
}

parser_error_t parseScrollMouseMacroAction(config_buffer_t *buffer, macro_action_t *macroAction)
{
    int16_t x = readInt16(buffer);
    int16_t y = readInt16(buffer);

    macroAction->type = MacroActionType_ScrollMouse;
    macroAction->scrollMouse.x = x;
    macroAction->scrollMouse.y = y;
    return ParserError_Success;
}

parser_error_t parseDelayMacroAction(config_buffer_t *buffer, macro_action_t *macroAction)
{
    int16_t delay = readInt16(buffer);

    macroAction->type = MacroActionType_Delay;
    macroAction->delay.delay = delay;
    return ParserError_Success;
}

parser_error_t parseTextMacroAction(config_buffer_t *buffer, macro_action_t *macroAction)
{
    uint16_t textLen;
    const char *text = readString(buffer, &textLen);

    macroAction->type = MacroActionType_Text;
    macroAction->text.text = text;
    macroAction->text.textLen = textLen;
    return ParserError_Success;
}

parser_error_t ParseMacroAction(config_buffer_t *buffer, macro_action_t *macroAction)
{
    uint8_t macroActionType = readUInt8(buffer);

    switch (macroActionType) {
        case SerializedMacroActionType_KeyMacroAction ... SerializedMacroActionType_LastKeyMacroAction:
            return parseKeyMacroAction(buffer, macroAction, macroActionType);
        case SerializedMacroActionType_MouseButtonMacroAction ... SerializedMacroActionType_LastMouseButtonMacroAction:
            return parseMouseButtonMacroAction(buffer, macroAction, macroActionType);
        case SerializedMacroActionType_MoveMouseMacroAction:
            return parseMoveMouseMacroAction(buffer, macroAction);
        case SerializedMacroActionType_ScrollMouseMacroAction:
            return parseScrollMouseMacroAction(buffer, macroAction);
        case SerializedMacroActionType_DelayMacroAction:
            return parseDelayMacroAction(buffer, macroAction);
        case SerializedMacroActionType_TextMacroAction:
            return parseTextMacroAction(buffer, macroAction);
    }
    return ParserError_InvalidSerializedMacroActionType;
}

parser_error_t ParseMacro(config_buffer_t *buffer, uint8_t macroIdx)
{
    parser_error_t errorCode;
    uint16_t nameLen;
    bool isLooped = readBool(buffer);
    bool isPrivate = readBool(buffer);
    const char *name = readString(buffer, &nameLen);
    uint16_t macroActionsCount = readCompactLength(buffer);
    uint16_t firstMacroActionOffset = buffer->offset;
    macro_action_t dummyMacroAction;

    (void)isLooped;
    (void)isPrivate;
    (void)name;
    if (!ParserRunDry) {
        AllMacros[macroIdx].firstMacroActionOffset = firstMacroActionOffset;
        AllMacros[macroIdx].macroActionsCount = macroActionsCount;
    }
    for (uint16_t i = 0; i < macroActionsCount; i++) {
        errorCode = ParseMacroAction(buffer, &dummyMacroAction);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
    }
    return ParserError_Success;
}

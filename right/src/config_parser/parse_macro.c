#include "parse_macro.h"
#include "config_globals.h"
#include "str_utils.h"
#include "macros.h"

parser_error_t parseKeyMacroAction(config_buffer_t *buffer, macro_action_t *macroAction, serialized_macro_action_type_t macroActionType)
{
    uint8_t keyMacroType = macroActionType - SerializedMacroActionType_KeyMacroAction;
    uint8_t action = keyMacroType & 0b11;
    uint8_t type;
    uint16_t scancode = 0;
    uint8_t modifierMask;

    keyMacroType >>= 2;
    type = keyMacroType & 0b11;
    keyMacroType >>= 2;
    if (keyMacroType & 0b10) {
        scancode = type == SerializedKeystrokeType_LongMedia ? ReadUInt16(buffer) : ReadUInt8(buffer);
    }
    modifierMask = keyMacroType & 0b01 ? ReadUInt8(buffer) : 0;
    macroAction->type = MacroActionType_Key;
    macroAction->key.action = action;
    macroAction->key.type = type;
    macroAction->key.scancode = scancode;
    macroAction->key.outputModMask = modifierMask;
    macroAction->key.inputModMask = 0;
    macroAction->key.stickyModMask = 0;
    return ParserError_Success;
}

parser_error_t parseMouseButtonMacroAction(config_buffer_t *buffer, macro_action_t *macroAction, serialized_macro_action_type_t macroActionType)
{
    uint8_t action = macroActionType - SerializedMacroActionType_MouseButtonMacroAction;
    uint8_t mouseButtonsMask = ReadUInt8(buffer);

    macroAction->type = MacroActionType_MouseButton;
    macroAction->mouseButton.action = action;
    macroAction->mouseButton.mouseButtonsMask = mouseButtonsMask;
    return ParserError_Success;
}

parser_error_t parseMoveMouseMacroAction(config_buffer_t *buffer, macro_action_t *macroAction)
{
    int16_t x = ReadInt16(buffer);
    int16_t y = ReadInt16(buffer);

    macroAction->type = MacroActionType_MoveMouse;
    macroAction->moveMouse.x = x;
    macroAction->moveMouse.y = y;
    return ParserError_Success;
}

parser_error_t parseScrollMouseMacroAction(config_buffer_t *buffer, macro_action_t *macroAction)
{
    int16_t x = ReadInt16(buffer);
    int16_t y = ReadInt16(buffer);

    macroAction->type = MacroActionType_ScrollMouse;
    macroAction->scrollMouse.x = x;
    macroAction->scrollMouse.y = y;
    return ParserError_Success;
}

parser_error_t parseDelayMacroAction(config_buffer_t *buffer, macro_action_t *macroAction)
{
    uint16_t delay = ReadUInt16(buffer);

    macroAction->type = MacroActionType_Delay;
    macroAction->delay.delay = delay;
    return ParserError_Success;
}

parser_error_t parseTextMacroAction(config_buffer_t *buffer, macro_action_t *macroAction)
{
    uint16_t textLen;
    const char *text = ReadString(buffer, &textLen);

    macroAction->type = MacroActionType_Text;
    macroAction->text.text = text;
    macroAction->text.textLen = textLen;

    return ParserError_Success;
}


parser_error_t parseCommandMacroAction(config_buffer_t *buffer, macro_action_t *macroAction)
{
    uint16_t textLen;
    const char *text = ReadString(buffer, &textLen);

    macroAction->type = MacroActionType_Command;
    macroAction->cmd.text = text;
    macroAction->cmd.textLen = textLen;
    macroAction->cmd.cmdCount = CountCommands(macroAction->cmd.text, macroAction->cmd.textLen);

    return ParserError_Success;
}

parser_error_t ParseMacroAction(config_buffer_t *buffer, macro_action_t *macroAction)
{
    uint8_t macroActionType = ReadUInt8(buffer);

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
        case SerializedMacroActionType_CommandMacroAction:
            return parseCommandMacroAction(buffer, macroAction);
    }
    return ParserError_InvalidSerializedMacroActionType;
}

void FindMacroName(const macro_reference_t* macro, const char** name, const char** nameEnd)
{
    uint16_t nameLen;
    config_buffer_t buffer = ValidatedUserConfigBuffer;
    buffer.offset = macro->firstMacroActionOffset - macro->macroNameOffset;
    *name = ReadString(&buffer, &nameLen);
    *nameEnd = *name + nameLen;
}

uint8_t FindMacroIndexByName(const char* name, const char* nameEnd, bool reportIfFailed)
{
    for (int i = 0; i < AllMacrosCount; i++) {
        const char *thisName, *thisNameEnd;
        FindMacroName(&AllMacros[i], &thisName, &thisNameEnd);
        if(StrEqual(name, nameEnd, thisName, thisNameEnd)) {
            return i;
        }
    }
    if (reportIfFailed) {
        Macros_ReportError("Macro name not found", name, nameEnd);
    }
    return 255;
}


parser_error_t ParseMacro(config_buffer_t *buffer, uint8_t macroIdx)
{
    parser_error_t errorCode;
    uint16_t nameLen;
    bool isLooped = ReadBool(buffer);
    bool isPrivate = ReadBool(buffer);
    uint16_t nameOffset = buffer->offset;
    const char *name = ReadString(buffer, &nameLen);
    uint16_t macroActionsCount = ReadCompactLength(buffer);
    uint16_t firstMacroActionOffset = buffer->offset;
    uint16_t relativeNameOffset = firstMacroActionOffset - nameOffset;
    macro_action_t dummyMacroAction;

    (void)isLooped;
    (void)isPrivate;
    (void)name;
    if (!ParserRunDry) {
        AllMacros[macroIdx].firstMacroActionOffset = firstMacroActionOffset;
        AllMacros[macroIdx].macroActionsCount = macroActionsCount;
        AllMacros[macroIdx].macroNameOffset = relativeNameOffset;
    }
    for (uint16_t i = 0; i < macroActionsCount; i++) {
        errorCode = ParseMacroAction(buffer, &dummyMacroAction);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
    }
    return ParserError_Success;
}

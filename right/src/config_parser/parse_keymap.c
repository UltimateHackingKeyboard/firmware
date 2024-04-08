#include <string.h>
#include "config_parser/config_globals.h"
#include "config_parser/parse_config.h"
#include "config_parser/parse_keymap.h"
#include "key_action.h"
#include "keymap.h"
#include "layer.h"
#include "ledmap.h"

static uint8_t tempKeymapCount;
static uint8_t tempMacroCount;

static void parseKeyActionColor(key_action_t *keyAction, config_buffer_t *buffer)
{
    if (PerKeyRgbPresent) {
        keyAction->color.red = ReadUInt8(buffer);
        keyAction->color.green = ReadUInt8(buffer);
        keyAction->color.blue = ReadUInt8(buffer);
        keyAction->colorOverridden = false;
    } else {
        keyAction->color.red = 0;
        keyAction->color.green = 0;
        keyAction->color.blue = 0;
        keyAction->colorOverridden = false;
    }
}

static parser_error_t parseNoneAction(key_action_t *keyAction, config_buffer_t *buffer)
{
    keyAction->type = KeyActionType_None;
    parseKeyActionColor(keyAction, buffer);
    return ParserError_Success;
}

static parser_error_t parseKeyStrokeAction(key_action_t *keyAction, uint8_t keyStrokeAction, config_buffer_t *buffer)
{
    uint8_t keystrokeType = (SERIALIZED_KEYSTROKE_TYPE_MASK_KEYSTROKE_TYPE & keyStrokeAction) >> SERIALIZED_KEYSTROKE_TYPE_OFFSET_KEYSTROKE_TYPE;

    keyAction->type = KeyActionType_Keystroke;
    switch (keystrokeType) {
        case SerializedKeystrokeType_Basic:
            keyAction->keystroke.keystrokeType = KeystrokeType_Basic;
            break;
        case SerializedKeystrokeType_ShortMedia:
        case SerializedKeystrokeType_LongMedia:
            keyAction->keystroke.keystrokeType = KeystrokeType_Media;
            break;
        case SerializedKeystrokeType_System:
            keyAction->keystroke.keystrokeType = KeystrokeType_System;
            break;
        default:
            return ParserError_InvalidSerializedKeystrokeType;
    }
    keyAction->keystroke.scancode = keyStrokeAction & SERIALIZED_KEYSTROKE_TYPE_MASK_HAS_SCANCODE
        ? keystrokeType == SerializedKeystrokeType_LongMedia ? ReadUInt16(buffer) : ReadUInt8(buffer)
        : 0;
    keyAction->keystroke.modifiers = keyStrokeAction & SERIALIZED_KEYSTROKE_TYPE_MASK_HAS_MODIFIERS
        ? ReadUInt8(buffer)
        : 0;
    keyAction->keystroke.secondaryRole = keyStrokeAction & SERIALIZED_KEYSTROKE_TYPE_MASK_HAS_LONGPRESS
        ? ReadUInt8(buffer) + 1
        : 0;
    parseKeyActionColor(keyAction, buffer);
    return ParserError_Success;
}

static parser_error_t parseSwitchLayerAction(key_action_t *keyAction, config_buffer_t *buffer)
{
    uint8_t layer = ReadUInt8(buffer) + 1;
    switch_layer_mode_t mode = ReadUInt8(buffer);

    keyAction->type = KeyActionType_SwitchLayer;
    keyAction->switchLayer.layer = layer;
    keyAction->switchLayer.mode = mode;
    parseKeyActionColor(keyAction, buffer);
    return ParserError_Success;
}

static parser_error_t parseSwitchKeymapAction(key_action_t *keyAction, config_buffer_t *buffer)
{
    uint8_t keymapIndex = ReadUInt8(buffer);

    if (keymapIndex >= tempKeymapCount) {
        return ParserError_InvalidSerializedSwitchKeymapAction;
    }
    keyAction->type = KeyActionType_SwitchKeymap;
    keyAction->switchKeymap.keymapId = keymapIndex;
    parseKeyActionColor(keyAction, buffer);
    return ParserError_Success;
}

static parser_error_t parsePlayMacroAction(key_action_t *keyAction, config_buffer_t *buffer)
{
    uint8_t macroIndex = ReadUInt8(buffer);

    if (macroIndex >= tempMacroCount) {
        return ParserError_InvalidSerializedPlayMacroAction;
    }
    keyAction->type = KeyActionType_PlayMacro;
    keyAction->playMacro.macroId = macroIndex;
    parseKeyActionColor(keyAction, buffer);
    return ParserError_Success;
}

static parser_error_t parseMouseAction(key_action_t *keyAction, config_buffer_t *buffer)
{
    keyAction->type = KeyActionType_Mouse;

    uint8_t mouseAction = ReadUInt8(buffer);
    if (mouseAction > SerializedMouseAction_Last) {
        return ParserError_InvalidSerializedMouseAction;
    }

    memset(&keyAction->mouseAction, 0, sizeof keyAction->mouseAction);
    keyAction->mouseAction = mouseAction;

    parseKeyActionColor(keyAction, buffer);

    return ParserError_Success;
}

static parser_error_t parseKeyAction(key_action_t *keyAction, config_buffer_t *buffer, parse_mode_t parseMode)
{
    uint8_t keyActionType = ReadUInt8(buffer);
    key_action_t dummyKeyAction;

    if(parseMode == ParseMode_DryRun) {
        keyAction = &dummyKeyAction;
    } else if (parseMode == ParseMode_Overlay && keyActionType == SerializedKeyActionType_None) {
        keyAction = &dummyKeyAction;
    }

    switch (keyActionType) {
        case SerializedKeyActionType_None:
            return parseNoneAction(keyAction, buffer);
        case SerializedKeyActionType_KeyStroke ... SerializedKeyActionType_LastKeyStroke:
            return parseKeyStrokeAction(keyAction, keyActionType, buffer);
        case SerializedKeyActionType_SwitchLayer:
            return parseSwitchLayerAction(keyAction, buffer);
        case SerializedKeyActionType_SwitchKeymap:
            return parseSwitchKeymapAction(keyAction, buffer);
        case SerializedKeyActionType_Mouse:
            return parseMouseAction(keyAction, buffer);
        case SerializedKeyActionType_PlayMacro:
            return parsePlayMacroAction(keyAction, buffer);
    }
    return ParserError_InvalidSerializedKeyActionType;
}

static parser_error_t parseKeyActions(uint8_t targetLayer, config_buffer_t *buffer, uint8_t moduleId, parse_mode_t parseMode)
{
    parser_error_t errorCode;
    uint16_t actionCount = ReadCompactLength(buffer);

    if (actionCount > MAX_KEY_COUNT_PER_MODULE) {
        return ParserError_InvalidActionCount;
    }
    if (DEVICE_IS_UHK80_RIGHT && moduleId == ModuleId_LeftKeyboardHalf) {
        //assume that left half is connected
        parseMode = parseMode;
    } else {
        parseMode = IsModuleAttached(moduleId) ? parseMode : ParseMode_DryRun;
    }
    slot_t slotId = ModuleIdToSlotId(moduleId);
    for (uint8_t actionIdx = 0; actionIdx < actionCount; actionIdx++) {
        errorCode = parseKeyAction(&CurrentKeymap[targetLayer][slotId][actionIdx], buffer, parseMode);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
    }
    /* default second touchpad action to right button */
    if (parseMode != ParseMode_DryRun && moduleId == ModuleId_TouchpadRight) {
        CurrentKeymap[targetLayer][slotId][1].type = KeyActionType_Mouse;
        CurrentKeymap[targetLayer][slotId][1].mouseAction = SerializedMouseAction_RightClick;
    }
    return ParserError_Success;
}

static parser_error_t parseModule(config_buffer_t *buffer, uint8_t layer, parse_mode_t parseMode)
{
    uint8_t moduleId = ReadUInt8(buffer);
    return parseKeyActions(layer, buffer, moduleId, parseMode);
}

static parser_error_t parseLayerId(config_buffer_t *buffer, uint8_t layer, layer_id_t* parsedLayerId)
{
    if(DataModelMajorVersion >= 5) {
        uint8_t layerId = ReadUInt8(buffer);
        switch(layerId) {
        case SerializedLayerName_base:
            *parsedLayerId = LayerId_Base;
            break;
        case SerializedLayerName_mod ... SerializedLayerName_super:
            *parsedLayerId = layerId + 1;
            break;
        default:
            return ParserError_InvalidLayerId;
        }
    } else {
        *parsedLayerId = layer;
    }

    return ParserError_Success;
}

static parser_error_t parseLayer(config_buffer_t *buffer, uint8_t layer, parse_mode_t parseMode)
{
    if (parseMode != ParseMode_DryRun) {
        LayerConfig[layer].layerIsDefined = true;
    }

    parser_error_t errorCode;
    uint16_t moduleCount = ReadCompactLength(buffer);

    if (moduleCount > ModuleId_AllCount) {
        return ParserError_InvalidModuleCount;
    }
    for (uint8_t moduleIdx = 0; moduleIdx < moduleCount; moduleIdx++) {
        errorCode = parseModule(buffer, layer, parseMode);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
    }
    return ParserError_Success;
}

void interpretConfig(parse_config_t parseConfig, layer_id_t srcLayer, layer_id_t* dstLayer, parse_mode_t* parseMode)
{
    switch (parseConfig.mode) {
        case ParseKeymapMode_DryRun:
            *dstLayer = srcLayer;
            *parseMode = ParseMode_DryRun;
            break;
        case ParseKeymapMode_FullRun:
            *dstLayer = srcLayer;
            *parseMode = ParseMode_FullRun;
            break;
        case ParseKeymapMode_OverlayKeymap:
            *dstLayer = srcLayer;
            *parseMode = ParseMode_Overlay;
            break;
        case ParseKeymapMode_OverlayLayer:
            if (parseConfig.srcLayer == srcLayer) {
                *dstLayer = parseConfig.dstLayer;
                *parseMode = ParseMode_Overlay;
            } else {
                *dstLayer = srcLayer;
                *parseMode = ParseMode_DryRun;
            }
            break;
        case ParseKeymapMode_ReplaceLayer:
            if (parseConfig.srcLayer == srcLayer) {
                *dstLayer = parseConfig.dstLayer;
                *parseMode = ParseMode_FullRun;
            } else {
                *dstLayer = srcLayer;
                *parseMode = ParseMode_DryRun;
            }
            break;
    }
}

parser_error_t ParseKeymap(config_buffer_t *buffer, uint8_t keymapIdx, uint8_t keymapCount, uint8_t macroCount, parse_config_t parseConfig)
{
    uint16_t offset = buffer->offset;
    parser_error_t errorCode;
    uint16_t abbreviationLen;
    uint16_t nameLen;
    uint16_t descriptionLen;
    const char *abbreviation = ReadString(buffer, &abbreviationLen);
    bool isDefault = ReadBool(buffer);
    const char *name = ReadString(buffer, &nameLen);
    const char *description = ReadString(buffer, &descriptionLen);
    uint16_t layerCount = ReadCompactLength(buffer);

    (void)name;
    (void)description;
    if (!abbreviationLen || abbreviationLen > 3) {
        return ParserError_InvalidAbbreviationLen;
    }
    if (layerCount > LayerId_Count) {
        return ParserError_InvalidLayerCount;
    }
    if (parseConfig.mode == ParseKeymapMode_FullRun) {
        AllKeymaps[keymapIdx].abbreviation = abbreviation;
        AllKeymaps[keymapIdx].abbreviationLen = abbreviationLen;
        AllKeymaps[keymapIdx].offset = offset;
        for (uint8_t layerIdx = 0; layerIdx < LayerId_Count; layerIdx++) {
            LayerConfig[layerIdx].layerIsDefined = false;
        }
        if (isDefault) {
            DefaultKeymapIndex = keymapIdx;
        }

        /* Clear the actions, since Agent may specify just part of a layer. */
        memset(CurrentKeymap, 0, sizeof CurrentKeymap);
    }
    tempKeymapCount = keymapCount;
    tempMacroCount = macroCount;
    for (uint8_t layerIdx = 0; layerIdx < layerCount; layerIdx++) {
        parse_mode_t parseMode;
        layer_id_t dstLayer;
        layer_id_t srcLayer;
        errorCode = parseLayerId(buffer, layerIdx, &srcLayer);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
        interpretConfig(parseConfig, srcLayer, &dstLayer, &parseMode);
        errorCode = parseLayer(buffer, dstLayer, parseMode);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
    }
    return ParserError_Success;
}

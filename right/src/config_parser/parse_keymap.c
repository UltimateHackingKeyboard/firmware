#include <string.h>
#include "config_parser/config_globals.h"
#include "config_parser/parse_config.h"
#include "config_parser/parse_keymap.h"
#include "key_action.h"
#include "keymap.h"
#include "layer.h"
#include "ledmap.h"
#include "led_display.h"
#include "config_manager.h"
#include "parse_keymap.h"
#include "slave_protocol.h"
#include "slot.h"
#include "slave_drivers/uhk_module_driver.h"
#include "error_reporting.h"

#ifdef __ZEPHYR__
#include "state_sync.h"
#endif

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
            ConfigParser_Error(buffer, "Invalid keystroke type: %d", keystrokeType);
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
        ConfigParser_Error(buffer, "Invalid keymap index: %d", keymapIndex);
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
        ConfigParser_Error(buffer, "Invalid macro index: %d", macroIndex);
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
        ConfigParser_Error(buffer, "Invalid mouse action: %d", mouseAction);
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

    ConfigParser_Error(buffer, "Invalid key action type: %d", keyActionType);
    return ParserError_InvalidSerializedKeyActionType;
}

static parser_error_t parseKeyActions(uint8_t targetLayer, config_buffer_t *buffer, uint8_t moduleId, parse_mode_t parseMode)
{
    parser_error_t errorCode;
    uint16_t actionCount = ReadCompactLength(buffer);

    if (actionCount > MAX_KEY_COUNT_PER_MODULE) {
        ConfigParser_Error(buffer, "Invalid action count: %d", actionCount);
        return ParserError_InvalidActionCount;
    }
    if (moduleId == ModuleId_LeftKeyboardHalf || moduleId == ModuleId_KeyClusterLeft) {
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
    if(DataModelVersion.major >= 5) {
        uint8_t layerId = ReadUInt8(buffer);
        switch(layerId) {
        case SerializedLayerName_base:
            *parsedLayerId = LayerId_Base;
            break;
        case SerializedLayerName_mod ... SerializedLayerName_super:
            *parsedLayerId = layerId + 1;
            break;
        default:
            ConfigParser_Error(buffer, "Invalid layer id: %d", layerId);
            return ParserError_InvalidLayerId;
        }
    } else {
        *parsedLayerId = layer;
    }

    return ParserError_Success;
}

static void applyDefaultRightModuleActions(uint8_t layer, parse_mode_t parseMode) {
    if(parseMode != ParseMode_DryRun && layer <= LayerId_RegularLast) {
        CurrentKeymap[layer][SlotId_RightModule][0] = (key_action_t){ .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_LeftClick };
        CurrentKeymap[layer][SlotId_RightModule][1] = (key_action_t){ .type = KeyActionType_Mouse, .mouseAction = SerializedMouseAction_RightClick };
    }
}

static void applyDefaultLeftModuleActions(uint8_t layer, parse_mode_t parseMode) {
    if(parseMode != ParseMode_DryRun && layer <= LayerId_RegularLast) {
        CurrentKeymap[layer][SlotId_LeftModule][0] = (key_action_t){ .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_DELETE }};
        CurrentKeymap[layer][SlotId_LeftModule][1] = (key_action_t){ .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_BACKSPACE }};
        CurrentKeymap[layer][SlotId_LeftModule][2] = (key_action_t){ .type = KeyActionType_Keystroke, .keystroke = { .scancode = HID_KEYBOARD_SC_ENTER }};
    }
}

static parser_error_t parseLayer(config_buffer_t *buffer, uint8_t layer, parse_mode_t parseMode)
{
    if (parseMode != ParseMode_DryRun) {
        Cfg.LayerConfig[layer].layerIsDefined = true;
    }

    parser_error_t errorCode;
    uint16_t moduleCount = ReadCompactLength(buffer);

    if (moduleCount > ModuleId_AllCount) {
        ConfigParser_Error(buffer, "Invalid module count: %d", moduleCount);
        return ParserError_InvalidModuleCount;
    }
    for (uint8_t moduleIdx = 0; moduleIdx < moduleCount; moduleIdx++) {
        errorCode = parseModule(buffer, layer, parseMode);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
    }

    // if current config doesn't configuration of the connected module, fill in hardwired values
    bool rightUhkModuleUnmapped = moduleCount <= UhkModuleStates[UhkModuleSlaveDriver_SlotIdToDriverId(SlotId_RightModule)].moduleId;
    bool touchpadUnmapped = moduleCount <= ModuleId_TouchpadRight && IsModuleAttached(ModuleId_TouchpadRight);
    if (rightUhkModuleUnmapped || touchpadUnmapped) {
        applyDefaultRightModuleActions(layer, parseMode);
    }

    // if current config doesn't configuration of the connected module, fill in hardwired values
    if (moduleCount <= UhkModuleStates[UhkModuleSlaveDriver_SlotIdToDriverId(SlotId_LeftModule)].moduleId) {
        applyDefaultLeftModuleActions(layer, parseMode);
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

parser_error_t ParseKeymapName(config_buffer_t* buffer, const char** name, uint16_t* len)
{
    uint16_t abbreviationLen;
    ReadString(buffer, &abbreviationLen);
    ReadBool(buffer);
    *name = ReadString(buffer, len);
    return ParserError_Success;
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
        ConfigParser_Error(buffer, "Invalid abbreviation length: %d", abbreviationLen);
        return ParserError_InvalidAbbreviationLen;
    }
    if (layerCount > LayerId_Count) {
        ConfigParser_Error(buffer, "Invalid layer count: %d", layerCount);
        return ParserError_InvalidLayerCount;
    }
    if (parseConfig.mode == ParseKeymapMode_FullRun) {
        AllKeymaps[keymapIdx].abbreviation = abbreviation;
        AllKeymaps[keymapIdx].abbreviationLen = abbreviationLen;
        AllKeymaps[keymapIdx].offset = offset;
        for (uint8_t layerIdx = 0; layerIdx < LayerId_Count; layerIdx++) {
            Cfg.LayerConfig[layerIdx].layerIsDefined = false;
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

#ifdef __ZEPHYR__
    if (parseConfig.mode == ParseKeymapMode_FullRun || parseConfig.mode == ParseKeymapMode_OverlayKeymap) {
        for (uint8_t layerId = 0; layerId < LayerId_Count; layerId++) {
            StateSync_UpdateLayer(layerId, Cfg.LayerConfig[layerId].layerIsDefined);
        }
    } else if (parseConfig.mode != ParseKeymapMode_DryRun) {
        StateSync_UpdateLayer(parseConfig.dstLayer, true);
    }
#endif

    return ParserError_Success;
}

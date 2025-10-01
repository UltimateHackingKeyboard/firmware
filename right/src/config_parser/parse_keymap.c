#include <string.h>
#include "basic_types.h"
#include "config_parser/config_globals.h"
#include "config_parser/parse_config.h"
#include "config_parser/parse_keymap.h"
#include "key_action.h"
#include "keymap.h"
#include "layer.h"
#include "ledmap.h"
#include "led_display.h"
#include "config_manager.h"
#include "module.h"
#include "parse_keymap.h"
#include "slave_protocol.h"
#include "slot.h"
#include "slave_drivers/uhk_module_driver.h"
#include "error_reporting.h"
#include "host_connection.h"
#include "macros/status_buffer.h"

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

static parser_error_t parseConnectionsAction(key_action_t *keyAction, config_buffer_t *buffer) {
    uint8_t connectionCommand = ReadUInt8(buffer);

    keyAction->type = KeyActionType_Connections;

    switch (connectionCommand) {
        case SerializedConnectionAction_SwitchByHostConnectionId: {
            uint8_t hostConnectionId = ReadUInt8(buffer);
            if (hostConnectionId < SERIALIZED_HOST_CONNECTION_COUNT_MAX) {
                keyAction->connections.command = ConnectionAction_SwitchByHostConnectionId;
                keyAction->connections.hostConnectionId = hostConnectionId;
            } else {
                ConfigParser_Error(buffer, "Invalid host connection id: %d", hostConnectionId);
                return ParserError_InvalidHostConnectionId;
            }
                                                                  }
            break;
        case SerializedConnectionAction_Last:
            keyAction->connections.command = ConnectionAction_Last;
            keyAction->connections.hostConnectionId = 0;
            break;
        case SerializedConnectionAction_Next:
            keyAction->connections.command = ConnectionAction_Next;
            keyAction->connections.hostConnectionId = 0;
            break;
        case SerializedConnectionAction_Previous:
            keyAction->connections.command = ConnectionAction_Previous;
            keyAction->connections.hostConnectionId = 0;
            break;
        case SerializedConnectionAction_ToggleAdvertisement:
            keyAction->connections.command = ConnectionAction_ToggleAdvertisement;
            keyAction->connections.hostConnectionId = 0;
            break;
        case SerializedConnectionAction_TogglePairing:
            keyAction->connections.command = ConnectionAction_TogglePairing;
            keyAction->connections.hostConnectionId = 0;
            break;
        default:
            ConfigParser_Error(buffer, "Invalid serialized connection action: %d", connectionCommand);
            return ParserError_InvalidSerializedConnectionAction;

            break;
    }

    parseKeyActionColor(keyAction, buffer);
    return ParserError_Success;
}

static parser_error_t parseOtherAction(key_action_t *keyAction, config_buffer_t *buffer) {
    uint8_t otherAction = ReadUInt8(buffer);

    switch (otherAction) {
        case SerializedOtherAction_Sleep:
            keyAction->type = KeyActionType_Other;
            keyAction->other.actionSubtype = OtherAction_Sleep;
            break;
        case SerializedOtherAction_Lock:
            keyAction->type = KeyActionType_Other;
            keyAction->other.actionSubtype = OtherAction_Lock;
            break;
        default:
            ConfigParser_Error(buffer, "Invalid other action: %d", otherAction);
            return ParserError_InvalidSerializedOtherAction;
    }

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

static void noneBlockAction(key_action_t *keyAction, rgb_t* color, parse_mode_t parseMode)
{
    if(parseMode == ParseMode_DryRun || parseMode == ParseMode_Overlay) {
        return;
    }

    keyAction->type = KeyActionType_None;
    keyAction->color.red = color->red;
    keyAction->color.green = color->green;
    keyAction->color.blue = color->blue;
    keyAction->colorOverridden = false;
}

static parser_error_t parseNoneBlock(key_action_t *keyAction, config_buffer_t *buffer, uint8_t *actionCountToNone, rgb_t* color, parse_mode_t parseMode)
{
    if (actionCountToNone != NULL) {
        *actionCountToNone = ReadUInt8(buffer);

        // Parse color
        key_action_t dummyAction;
        parseKeyActionColor(&dummyAction, buffer);
        color->red = dummyAction.color.red;
        color->green = dummyAction.color.green;
        color->blue = dummyAction.color.blue;
    }

    if (*actionCountToNone > 0) {
        noneBlockAction(keyAction, color, parseMode);
    }

    return ParserError_Success;
}

static parser_error_t skipArgument(config_buffer_t *buffer, bool *wasArgument) {
    if (wasArgument != NULL) {
        *wasArgument = true;
    }

    uint16_t length = ReadCompactLength(buffer);
    for (uint16_t i = 0; i < length; i++) {
        //we don't care here, just discard them
        ReadUInt8(buffer);
    }
    return ParserError_Success;
}

static parser_error_t parseKeyAction(key_action_t *keyAction, config_buffer_t *buffer, parse_mode_t parseMode, uint8_t *actionCountToNone, rgb_t* noneBlockColor, bool *wasArgument)
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
        case SerializedKeyActionType_Connections:
            return parseConnectionsAction(keyAction, buffer);
        case SerializedKeyActionType_Other:
            return parseOtherAction(keyAction, buffer);
        case SerializedKeyActionType_NoneBlock:
            return parseNoneBlock(keyAction, buffer, actionCountToNone, noneBlockColor, parseMode);
        case SerializedKeyActionType_Label:
        case SerializedKeyActionType_Argument:
            return skipArgument(buffer, wasArgument);
    }

    ConfigParser_Error(buffer, "Invalid key action type: %d", keyActionType);
    return ParserError_InvalidSerializedKeyActionType;
}

static parser_error_t parseKeyActions(uint8_t targetLayer, config_buffer_t *buffer, uint8_t moduleId, parse_mode_t parseMode)
{
    parser_error_t errorCode;
    uint16_t actionCount = ReadCompactLength(buffer);

    if (moduleId == ModuleId_LeftKeyboardHalf || moduleId == ModuleId_KeyClusterLeft) {
        parseMode = parseMode;
    } else {
        parseMode = IsModuleAttached(moduleId) ? parseMode : ParseMode_DryRun;
    }
    slot_t slotId = ModuleIdToSlotId(moduleId);
    uint8_t noneBlockUntil = 0;
    rgb_t noneBlockColor = {0, 0, 0};
    for (uint8_t actionIdx = 0; actionIdx < actionCount; actionIdx++) {
        key_action_t dummyKeyAction;
        key_action_t *keyAction = actionIdx < MAX_KEY_COUNT_PER_MODULE ? &CurrentKeymap[targetLayer][slotId][actionIdx] : &dummyKeyAction;

        if (actionIdx < noneBlockUntil) {
            noneBlockAction(keyAction, &noneBlockColor, parseMode);
        } else {
            bool wasArgument = false;
            uint8_t actionCountToNone = 0;
            errorCode = parseKeyAction(keyAction, buffer, parseMode, &actionCountToNone, &noneBlockColor, &wasArgument);

            if (errorCode != ParserError_Success) {
                return errorCode;
            }

            noneBlockUntil = actionIdx + actionCountToNone;

            if (wasArgument) {
                actionIdx--;
            }
        }
    }
    /* default second touchpad action to right button */
    if (parseMode != ParseMode_DryRun && moduleId == ModuleId_TouchpadRight) {
        CurrentKeymap[targetLayer][slotId][1].type = KeyActionType_Mouse;
        CurrentKeymap[targetLayer][slotId][1].mouseAction = SerializedMouseAction_RightClick;
    }
    return ParserError_Success;
}

static parser_error_t parseModule(config_buffer_t *buffer, uint8_t moduleId, uint8_t layer, parse_mode_t parseMode)
{
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

    bool currentRightModuleWasRead = false;
    bool currentLeftModuleWasRead = false;

    if (moduleCount > ModuleId_AllModuleCount) {
        ConfigParser_Error(buffer, "Invalid module count: %d", moduleCount);
        return ParserError_InvalidModuleCount;
    }
    for (uint8_t moduleIdx = 0; moduleIdx < moduleCount; moduleIdx++) {
        uint8_t moduleId = ReadUInt8(buffer);

        currentRightModuleWasRead |= ModuleIdToSlotId(moduleId) == SlotId_RightModule && IsModuleAttached(moduleId);
        currentLeftModuleWasRead |= ModuleIdToSlotId(moduleId) == SlotId_LeftModule && IsModuleAttached(moduleId);

        errorCode = parseModule(buffer, moduleId, layer, parseMode);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
    }

    if (!currentRightModuleWasRead) {
        applyDefaultRightModuleActions(layer, parseMode);
    }

    if (!currentLeftModuleWasRead) {
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
        case ParseKeymapMode_ReplaceKeymap:
            *dstLayer = srcLayer;
            *parseMode = ParseMode_FullRun;
            break;
        default:
            {
                Macros_ReportErrorPrintf(NULL, "Unrecognized parse mode: %d\n", parseConfig.mode);
                *dstLayer = LayerId_Base;
                *parseMode = ParseMode_DryRun;
            }
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
    if (parseConfig.mode == ParseKeymapMode_FullRun || parseConfig.mode == ParseKeymapMode_OverlayKeymap || parseConfig.mode == ParseKeymapMode_ReplaceKeymap) {
        for (uint8_t layerId = 0; layerId < LayerId_Count; layerId++) {
            StateSync_UpdateLayer(layerId, Cfg.LayerConfig[layerId].layerIsDefined);
        }
    } else if (parseConfig.mode != ParseKeymapMode_DryRun) {
        StateSync_UpdateLayer(parseConfig.dstLayer, true);
    }
#endif

    return ParserError_Success;
}

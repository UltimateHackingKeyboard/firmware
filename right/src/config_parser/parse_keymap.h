#ifndef __PARSE_KEYMAP_H__
#define __PARSE_KEYMAP_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "layer.h"
    #include "parse_config.h"

// Macros:

    #define SERIALIZED_KEYSTROKE_TYPE_MASK_HAS_SCANCODE   0b00001
    #define SERIALIZED_KEYSTROKE_TYPE_MASK_HAS_MODIFIERS  0b00010
    #define SERIALIZED_KEYSTROKE_TYPE_MASK_HAS_LONGPRESS  0b00100
    #define SERIALIZED_KEYSTROKE_TYPE_MASK_KEYSTROKE_TYPE 0b11000
    #define SERIALIZED_KEYSTROKE_TYPE_OFFSET_KEYSTROKE_TYPE 3

// Typedefs:

    typedef enum {
        SerializedKeyActionType_None = 0,
        SerializedKeyActionType_KeyStroke = 1,
        SerializedKeyActionType_LastKeyStroke = 31,
        SerializedKeyActionType_SwitchLayer = 32,
        SerializedKeyActionType_SwitchKeymap = 33,
        SerializedKeyActionType_Mouse = 34,
        SerializedKeyActionType_PlayMacro = 35,
        SerializedKeyActionType_Connections = 36,
        SerializedKeyActionType_Other = 37,
    } serialized_key_action_type_t;

    typedef enum {
        SerializedKeystrokeType_Basic,
        SerializedKeystrokeType_ShortMedia,
        SerializedKeystrokeType_LongMedia,
        SerializedKeystrokeType_System,
    } serialized_keystroke_type_t;

    typedef enum {
        SerializedMouseAction_LeftClick,
        SerializedMouseAction_MiddleClick,
        SerializedMouseAction_RightClick,
        SerializedMouseAction_MoveUp,
        SerializedMouseAction_MoveDown,
        SerializedMouseAction_MoveLeft,
        SerializedMouseAction_MoveRight,
        SerializedMouseAction_ScrollUp,
        SerializedMouseAction_ScrollDown,
        SerializedMouseAction_ScrollLeft,
        SerializedMouseAction_ScrollRight,
        SerializedMouseAction_Accelerate,
        SerializedMouseAction_Decelerate,
        SerializedMouseAction_Button_4,
        SerializedMouseAction_Button_5,
        SerializedMouseAction_Button_6,
        SerializedMouseAction_Button_7,
        SerializedMouseAction_Button_8,
        SerializedMouseAction_Button_9,
        SerializedMouseAction_Button_10,
        SerializedMouseAction_Button_11,
        SerializedMouseAction_Button_12,
        SerializedMouseAction_Button_13,
        SerializedMouseAction_Button_14,
        SerializedMouseAction_Button_15,
        SerializedMouseAction_Button_16,
        SerializedMouseAction_Button_17,
        SerializedMouseAction_Button_18,
        SerializedMouseAction_Button_19,
        SerializedMouseAction_Button_20,
        SerializedMouseAction_Button_Last = SerializedMouseAction_Button_20,
        SerializedMouseAction_Last = SerializedMouseAction_Button_20,
    } serialized_mouse_action_t;

    typedef enum {
        SerializedLayerName_mod,
        SerializedLayerName_fn,
        SerializedLayerName_mouse,
        SerializedLayerName_fn2,
        SerializedLayerName_fn3,
        SerializedLayerName_fn4,
        SerializedLayerName_fn5,
        SerializedLayerName_shift,
        SerializedLayerName_control,
        SerializedLayerName_alt,
        SerializedLayerName_super,
        SerializedLayerName_base = 255
    } serialized_layer_id;

    typedef enum {
        ParseMode_DryRun,
        ParseMode_FullRun,
        ParseMode_Overlay,
    } parse_mode_t;

    typedef enum {
        ParseKeymapMode_DryRun,
        ParseKeymapMode_FullRun,
        ParseKeymapMode_OverlayKeymap,
        ParseKeymapMode_OverlayLayer,
        ParseKeymapMode_ReplaceLayer,
    } parse_keymap_mode_t;

    typedef enum {
        SerializedConnectionAction_Last,
        SerializedConnectionAction_Next,
        SerializedConnectionAction_Previous,
        SerializedConnectionAction_SwitchByHostConnectionId,
    } serialized_connection_action_t;

    typedef enum {
        SerializedOtherAction_Sleep,
    } serialized_other_action_t;

    typedef struct {
        parse_keymap_mode_t mode;
        layer_id_t srcLayer;
        layer_id_t dstLayer;
    } ATTR_PACKED parse_config_t;

// Functions:

    parser_error_t ParseKeymap(config_buffer_t *buffer, uint8_t keymapIdx, uint8_t keymapCount, uint8_t macroCount, parse_config_t parseConfig);
    parser_error_t ParseKeymapName(config_buffer_t* buffer, const char** name, uint16_t* len);

#endif

#ifndef __PARSE_KEYMAP_H__
#define __PARSE_KEYMAP_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
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
        SerializedKeyActionType_SwitchLayer,
        SerializedKeyActionType_SwitchKeymap,
        SerializedKeyActionType_Mouse,
        SerializedKeyActionType_PlayMacro
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
        SerializedMouseAction_Last = SerializedMouseAction_Button_8,
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


// Functions:

    parser_error_t ParseKeymap(config_buffer_t *buffer, uint8_t keymapIdx, uint8_t keymapCount, uint8_t macroCount);

#endif

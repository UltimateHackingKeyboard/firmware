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
    } serialized_mouse_action_t;

// Functions:

    parser_error_t ParseKeymap(serialized_buffer_t *buffer);

#endif

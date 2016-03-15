#ifndef __ACTION_H__
#define __ACTION_H__

// Macros:

    // The value of action ID can be any valid HID_KEYBOARD_SC_* scancode constants of LUFA.
    // Hence, ACTION_ID_* values must not conflict with any of the HID_KEYBOARD_SC_* constants.
    #define ACTION_ID_NONE          0xFF
    #define ACTION_ID_SWITCH_LAYER  0xFE
    #define ACTION_ID_MOUSE         0xFD
    #define ACTION_ID_SWITCH_KEYMAP 0xFC
    #define ACTION_ID_PLAY_MACRO    0xFB

    #define ACTION_ARG_NONE 0

    #define ACTION_ARG_SWITCH_LAYER_MOD   0
    #define ACTION_ARG_SWITCH_LAYER_FN    1
    #define ACTION_ARG_SWITCH_LAYER_MOUSE 2

    #define ACTION_ARG_MOUSE_MOVE_UP      0
    #define ACTION_ARG_MOUSE_MOVE_DOWN    1
    #define ACTION_ARG_MOUSE_MOVE_LEFT    3
    #define ACTION_ARG_MOUSE_MOVE_RIGHT   4
    #define ACTION_ARG_MOUSE_CLICK_LEFT   5
    #define ACTION_ARG_MOUSE_CLICK_MIDDLE 6
    #define ACTION_ARG_MOUSE_CLICK_RIGHT  7
    #define ACTION_ARG_MOUSE_WHEEL_UP     8
    #define ACTION_ARG_MOUSE_WHEEL_DOWN   9
    #define ACTION_ARG_MOUSE_WHEEL_LEFT   10
    #define ACTION_ARG_MOUSE_WHEEL_RIGHT  11

// Typedefs:

    typedef struct {
        uint8_t id;
        uint8_t arg;
    } action_t;

#endif

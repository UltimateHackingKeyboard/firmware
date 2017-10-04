#ifndef __ACTION_H__
#define __ACTION_H__

// Includes:

    #include <stdint.h>
    #include "attributes.h"
    #include "lufa/HIDClassCommon.h"
    #include "usb_composite_device.h"
    #include "main.h"
    #include "module.h"

// Macros:

    #define MOUSE_WHEEL_SPEED   1
    #define MOUSE_WHEEL_DIVISOR 4

    #define MOUSE_MAX_SPEED           10
    #define MOUSE_SPEED_ACCEL_DIVISOR 50

// Typedefs:

    typedef enum {
        KeyActionType_None,
        KeyActionType_Keystroke,
        KeyActionType_Mouse,
        KeyActionType_SwitchLayer,
        KeyActionType_SwitchKeymap,
        KeyActionType_PlayMacro,
        KeyActionType_Test,
    } key_action_type_t;

    typedef enum {
        KeystrokeType_Basic,
        KeystrokeType_Media,
        KeystrokeType_System,
    } keystroke_type_t;

    typedef enum {
        MouseButton_Left   = 1 << 0,
        MouseButton_Right  = 1 << 1,
        MouseButton_Middle = 1 << 2,
        MouseButton_4      = 1 << 3,
        MouseButton_5      = 1 << 4,
        MouseButton_t      = 1 << 5,
    } mouse_button_t;

    typedef enum {
        MouseMove_Up    = 1 << 0,
        MouseMove_Down  = 1 << 1,
        MouseMove_Left  = 1 << 2,
        MouseMove_Right = 1 << 3,

        MouseMove_Accelerate = 1 << 4,
        MouseMove_Decelerate = 1 << 5,
    } mouse_move_action_t;

    typedef enum {
        MouseScroll_Up    = 1 << 0,
        MouseScroll_Down  = 1 << 1,
        MouseScroll_Left  = 1 << 2,
        MouseScroll_Right = 1 << 3,
    } mouse_scroll_t;

    typedef struct {
        uint8_t type;
        union {
            struct {
                keystroke_type_t keystrokeType;
                uint8_t longPressAction;
                uint8_t modifiers;
                uint16_t scancode;
            } ATTR_PACKED keystroke;
            struct {
                mouse_button_t buttonActions;
                mouse_scroll_t scrollActions;
                mouse_move_action_t moveActions;
            } ATTR_PACKED mouse;
            struct {
                bool isToggle;
                uint8_t layer;
            } ATTR_PACKED switchLayer;
            struct {
                uint8_t keymapId;
            } ATTR_PACKED switchKeymap;
            struct {
                uint8_t macroId;
            } ATTR_PACKED playMacro;
        };
    } ATTR_PACKED key_action_t;

// Variables:

    void UpdateActiveUsbReports(void);

#endif

#ifndef __KEY_ACTION_H__
#define __KEY_ACTION_H__

// Includes:

    #include <stdint.h>
    #include "attributes.h"
    #include "lufa/HIDClassCommon.h"
    #include "usb_composite_device.h"
    #include "module.h"
    #include "config_parser/parse_keymap.h"

// Typedefs:

    typedef enum {
        KeyActionType_None,
        KeyActionType_Keystroke,
        KeyActionType_Mouse,
        KeyActionType_SwitchLayer,
        KeyActionType_SwitchKeymap,
        KeyActionType_PlayMacro,
        KeyActionType_Gamepad,
    } key_action_type_t;

    typedef enum {
        KeystrokeType_Basic,
        KeystrokeType_Media,
        KeystrokeType_System,
    } keystroke_type_t;

    typedef enum {
        SwitchLayerMode_HoldAndDoubleTapToggle,
        SwitchLayerMode_Toggle,
        SwitchLayerMode_Hold,
    } switch_layer_mode_t;

    typedef enum {
        MouseButton_Left   = 1 << 0,
        MouseButton_Right  = 1 << 1,
        MouseButton_Middle = 1 << 2,
        MouseButton_4      = 1 << 3,
        MouseButton_5      = 1 << 4,
        MouseButton_6      = 1 << 5,
        MouseButton_7      = 1 << 6,
        MouseButton_8      = 1 << 7,
    } mouse_button_t;

    typedef struct {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    } rgb_t;

    typedef struct {
        uint8_t property;
    } gamepad_action_t;

    typedef struct {
        uint8_t type;
        union {
            struct {
                keystroke_type_t keystrokeType;
                uint8_t secondaryRole;
                uint8_t modifiers;
                uint16_t scancode;
            } ATTR_PACKED keystroke;
            serialized_mouse_action_t mouseAction;
            serialized_gamepad_action_t gamepadPropertyId;
            struct {
                switch_layer_mode_t mode;
                uint8_t layer;
            } ATTR_PACKED switchLayer;
            struct {
                uint8_t keymapId;
            } ATTR_PACKED switchKeymap;
            struct {
                uint8_t macroId;
            } ATTR_PACKED playMacro;
        };
        rgb_t color;
    } ATTR_PACKED key_action_t;

    typedef struct {
        key_action_t action;
        uint8_t modifierLayerMask;
    } ATTR_PACKED key_action_cached_t;

// Variables:

    void UpdateActiveUsbReports(void);

#endif

#ifndef __KEY_ACTION_H__
#define __KEY_ACTION_H__

// Includes:

    #include <stdint.h>
    #include "attributes.h"
    #include "lufa/HIDClassCommon.h"
#ifndef __ZEPHYR__
    #include "usb_composite_device.h"
#endif
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
        KeyActionType_Connections,
        KeyActionType_Other,
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
        ConnectionAction_Next,
        ConnectionAction_Previous,
        ConnectionAction_SwitchByHostConnectionId,
    } connection_action_t;

    typedef enum {
        OtherAction_Sleep,
    } other_action_t;

    typedef enum {
        MouseButton_Left   = 1 << 0,
        MouseButton_Right  = 1 << 1,
        MouseButton_Middle = 1 << 2,
        MouseButton_4      = 1 << 3,
        MouseButton_5      = 1 << 4,
        MouseButton_6      = 1 << 5,
        MouseButton_7      = 1 << 6,
        MouseButton_8      = 1 << 7,
        MouseButton_9      = 1 << 8,
        MouseButton_10      = 1 << 9,
        MouseButton_11      = 1 << 10,
        MouseButton_12      = 1 << 11,
        MouseButton_13      = 1 << 12,
        MouseButton_14      = 1 << 13,
        MouseButton_15      = 1 << 14,
        MouseButton_16      = 1 << 15,
        MouseButton_17      = 1 << 16,
        MouseButton_18      = 1 << 17,
        MouseButton_19      = 1 << 18,
        MouseButton_20      = 1 << 19,
    } mouse_button_t;

    typedef struct {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    } rgb_t;

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
            struct {
                connection_action_t command;
                uint8_t hostConnectionId;
            } ATTR_PACKED connections;
            struct {
                other_action_t actionSubtype;
            } ATTR_PACKED other;
        };
        rgb_t color;
        bool colorOverridden;
    } ATTR_PACKED key_action_t;

    typedef struct {
        key_action_t action;
        uint8_t modifierLayerMask;
    } ATTR_PACKED key_action_cached_t;

// Variables:

    void UpdateActiveUsbReports(void);

#endif

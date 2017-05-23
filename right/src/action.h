#ifndef __ACTION_H__
#define __ACTION_H__

#include <stdint.h>
#include "lufa/HIDClassCommon.h"
#include "usb_composite_device.h"
#include "main.h"

#include "module.h"

typedef enum {
    KEY_ACTION_NONE,
    KEY_ACTION_KEYSTROKE,
    KEY_ACTION_MOUSE,
    KEY_ACTION_SWITCH_LAYER,
    KEY_ACTION_SWITCH_KEYMAP,
    KEY_ACTION_PLAY_MACRO,
    KEY_ACTION_TEST,
} key_action_type_t;

typedef enum {
    KEYSTROKE_BASIC,
    KEYSTROKE_MEDIA,
    KEYSTROKE_SYSTEM,
} keystroke_type_t;

enum {
    MOUSE_BUTTON_LEFT   = (1 << 0),
    MOUSE_BUTTON_RIGHT  = (1 << 1),
    MOUSE_BUTTON_MIDDLE = (1 << 2),
    MOUSE_BUTTON_4      = (1 << 3),
    MOUSE_BUTTON_5      = (1 << 4),
    MOUSE_BUTTON_6      = (1 << 5),
};

enum {
    MOUSE_MOVE_UP    = (1 << 0),
    MOUSE_MOVE_DOWN  = (1 << 1),
    MOUSE_MOVE_LEFT  = (1 << 2),
    MOUSE_MOVE_RIGHT = (1 << 3),

    MOUSE_ACCELERATE = (1 << 4),
    MOUSE_DECELERATE = (1 << 5),
};

enum {
    MOUSE_SCROLL_UP    = (1 << 0),
    MOUSE_SCROLL_DOWN  = (1 << 1),
    MOUSE_SCROLL_LEFT  = (1 << 2),
    MOUSE_SCROLL_RIGHT = (1 << 3),
};

typedef enum {
    TestAction_DisableUsb,
    TestAction_DisableI2c,
    TestAction_DisableKeyMatrixScan,
    TestAction_DisableLedSdb,
    TestAction_DisableLedFetPwm,
    TestAction_DisableLedDriverPwm,
} test_action_t;

#define MOUSE_WHEEL_SPEED   1
#define MOUSE_WHEEL_DIVISOR 4

#define MOUSE_MAX_SPEED           10
#define MOUSE_SPEED_ACCEL_DIVISOR 50

typedef struct {
    uint8_t type;
    union {
        struct {
            keystroke_type_t keystrokeType;
            uint8_t longPressAction;
            uint8_t modifiers;
            uint16_t scancode;
        } __attribute__ ((packed)) keystroke;
        struct {
            uint8_t buttonActions;
            uint8_t scrollActions;
            uint8_t moveActions;
        } __attribute__ ((packed)) mouse;
        struct {
            bool isToggle;
            uint8_t layer;
        } __attribute__ ((packed)) switchLayer;
        struct {
            uint8_t keymapId;
        } __attribute__ ((packed)) switchKeymap;
        struct {
            uint16_t macroId;
        } __attribute__ ((packed)) playMacro;
        struct {
            test_action_t testAction;
        } __attribute__ ((packed)) test;
    };
} __attribute__ ((packed)) key_action_t;

extern void UpdateActiveUsbReports();

#endif

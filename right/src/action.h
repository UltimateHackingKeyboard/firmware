#ifndef __UHK_ACTION_H_
#define __UHK_ACTION_H_

#include <stdint.h>
#include "lufa/HIDClassCommon.h"
#include "usb_composite_device.h"
#include "main.h"

#include "module.h"

// Keyboard layout is a 2D array of scan codes.
//
// First dimension is the Key ID of a given key. Key IDs are the indices of the
// of the active keys of the key_matrix_t structure. In case of left half, an
// offset of 35 is added.
//
// For each Key ID, there are 4 different possible scan codes:
//      - default, when no modifiers are pressed
//      - mod layer
//      - fn layer
//      - mod+fn layer

typedef enum {
    KEY_ACTION_NONE,
    KEY_ACTION_KEYSTROKE,
    KEY_ACTION_MOUSE,
    KEY_ACTION_SWITCH_LAYER,
    KEY_ACTION_SWITCH_KEYMAP,
    KEY_ACTION_PLAY_MACRO,
} key_action_type_t;

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

#define MOUSE_WHEEL_SPEED   1
#define MOUSE_WHEEL_DIVISOR 4

#define MOUSE_MAX_SPEED           10
#define MOUSE_SPEED_ACCEL_DIVISOR 50

typedef struct {
    uint8_t type;
    union {
        struct {
            uint8_t longPress;
            uint8_t mods;
            uint8_t key;
        } __attribute__ ((packed)) keystroke;
        struct {
            uint8_t buttonActions; // bitfield
            uint8_t scrollActions; // bitfield
            uint8_t moveActions; // bitfield
        } __attribute__ ((packed)) mouse;
        struct {
            uint16_t __unused_bits:15;
            bool isToggle:1;
            uint8_t layer;
        } __attribute__ ((packed)) switchLayer;
        struct {
            uint16_t __unused_bits;
            uint8_t keymap;
        } __attribute__ ((packed)) switchKeymap;
        struct {
            uint8_t __unused_bits;
            uint16_t index;
        } __attribute__ ((packed)) playMacro;
    };
} __attribute__ ((packed)) key_action_t;

extern key_action_t CurrentKeymap[LAYER_COUNT][SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

void HandleKeyboardEvents(usb_keyboard_report_t *keyboardReport, usb_mouse_report_t *mouseReport);

#endif

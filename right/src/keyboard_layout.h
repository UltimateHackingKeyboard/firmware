#ifndef KEYBOARD_LAYOUT_H_
#define KEYBOARD_LAYOUT_H_

#include <stdint.h>
#include "lufa/HIDClassCommon.h"
#include "usb_composite_device.h"

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

#define KEY_STATE_COUNT (5*7)

typedef enum {
    UHK_KEY_NONE,
    UHK_KEY_SIMPLE,
    UHK_KEY_MOUSE,
    UHK_KEY_LAYER,
    UHK_KEY_LAYER_TOGGLE,
    UHK_KEY_KEYMAP,
    UHK_KEY_MACRO,
    UHK_KEY_LPRESSMOD,
    UHK_KEY_LPRESSLAYER,
} uhk_key_type_t;

typedef struct {
    uint8_t type;
    union {
        struct {
            uint8_t __unused_bits;
            uint8_t mods;
            uint8_t key;
        } __attribute__ ((packed)) simple;
        struct {
            uint8_t __unused_bits;
            uint8_t scrollActions; // bitfield
            uint8_t moveActions; // bitfield
        } __attribute__ ((packed)) mouse;
        struct {
            uint16_t __unused_bits;
            uint8_t target;
        } __attribute__ ((packed)) layer;
        struct {
            uint16_t __unused_bits;
            uint8_t target;
        } __attribute__ ((packed)) keymap;
        struct {
            uint8_t __unused_bits;
            uint16_t index;
        } __attribute__ ((packed)) macro;
        struct {
            uint8_t longPressMod; // single mod, or bitfield?
            uint8_t mods; // for the alternate action
            uint8_t key;
        } __attribute__ ((packed)) longpressMod;
        struct {
            uint8_t longPressLayer;
            uint8_t mods;
            uint8_t key;
        } __attribute__ ((packed)) longpressLayer;
    };
} __attribute__ ((packed)) uhk_key_t;

extern uint8_t prevKeyStates[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];
extern uhk_key_t CurrentKeymap[LAYER_COUNT][SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

void fillKeyboardReport(usb_keyboard_report_t *report, const uint8_t *leftKeyStates, const uint8_t *rightKeyStates);

#endif

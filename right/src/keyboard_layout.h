#ifndef KEYBOARD_LAYOUT_H_
#define KEYBOARD_LAYOUT_H_

#include <stdint.h>
#include "lufa/HIDClassCommon.h"
#include "usb_composite_device.h"

/**
 * Keyboard layout is a 2D array of scan codes.
 *
 * First dimension is the Key ID of a given key. Key IDs are the indices of the
 * of the active keys of the key_matrix_t structure. In case of left half, an
 * offset of 35 is added.
 *
 * For each Key ID, there are 4 different possible scan codes:
 * 		- default, when no modifiers are pressed
 * 		- mod layer
 * 		- fn layer
 * 		- mod+fn layer
 *
 */

#define KEY_STATE_COUNT (5*7)

#define LAYOUT_KEY_COUNT 70
#define LAYOUT_MOD_COUNT 4

#define LAYOUT_LEFT_OFFSET KEY_STATE_COUNT

typedef enum {
  UHK_KEY_SIMPLE,
  UHK_KEY_SWITCH_LAYER,
  UHK_KEY_MOUSE,
  UHK_KEY_SWITCH_KEYMAP,
  UHK_KEY_PLAY_MACRO,

  UHK_KEY_TEST,
} uhk_key_type_t;

typedef union {
  struct {
    uhk_key_type_t type;
    union {
      uint8_t value;
      uint8_t key;
    };
  };
  uint16_t raw;
} uhk_key_t;

#define Key_NoKey {.raw = 0}
#define KEYBOARD_LAYOUT(name) const uhk_key_t name[LAYOUT_KEY_COUNT][LAYOUT_MOD_COUNT]

#define KEYID_LEFT_MOD 33
#define KEYID_LEFT_FN 31
#define KEYID_RIGHT_FN 31

#define MODIFIER_MOD_PRESSED 1
#define MODIFIER_FN_PRESSED 2

void fillKeyboardReport(usb_keyboard_report_t *report, const uint8_t *leftKeyStates, const uint8_t *rightKeyStates, KEYBOARD_LAYOUT(layout));

#endif

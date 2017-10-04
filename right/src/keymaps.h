#ifndef __KEYMAPS_H__
#define __KEYMAPS_H__

// Includes:

    #include <stdint.h>
    #include "key_action.h"

// Macros:

    #define MAX_KEYMAP_NUM 255
    #define KEYMAP_ABBREVIATION_LENGTH 3

// Typedefs:

    typedef struct {
        const char *abbreviation;
        uint16_t offset;
        uint8_t abbreviationLen;
    } keymap_reference_t;

// Variables:

    extern keymap_reference_t AllKeymaps[MAX_KEYMAP_NUM];
    extern uint8_t AllKeymapsCount;
    extern uint8_t DefaultKeymapIndex;
    extern uint8_t CurrentKeymapIndex;
    extern key_action_t CurrentKeymap[LAYER_COUNT][SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

// Functions:

    void Keymaps_Switch(uint8_t index);

#endif

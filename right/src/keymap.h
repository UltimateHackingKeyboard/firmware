#ifndef __KEYMAPS_H__
#define __KEYMAPS_H__

// Includes:

    #include <stdint.h>
    #include "key_action.h"
    #include "str_utils.h"

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
    extern key_action_t CurrentKeymap[LayerId_Count][SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

// Functions:

    void SwitchKeymapById(uint8_t index, bool resetLayerStack);
    bool SwitchKeymapByAbbreviation(uint8_t length, const char *abbrev, bool resetLayerStack);
    uint8_t FindKeymapByAbbreviation(uint8_t length, const char *abbrev);

    string_segment_t GetKeymapName(uint8_t keymapId);

    void OverlayKeymap(uint8_t srcKeymap);
    void OverlayLayer(layer_id_t dstLayer, uint8_t srcKeymap, layer_id_t srcLayer);
    void ReplaceLayer(layer_id_t dstLayer, uint8_t srcKeymap, layer_id_t srcLayer);
    void ReplaceKeymap(uint8_t srcKeymap);

#endif

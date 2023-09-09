#ifndef __SEGMENT_DISPLAY_H__
#define __SEGMENT_DISPLAY_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "timer.h"

// Macros:

// Typedefs:

    typedef enum {
        SegmentDisplaySlot_Keymap,
        SegmentDisplaySlot_Macro,
        SegmentDisplaySlot_Recording,
        SegmentDisplaySlot_Error,
        SegmentDisplaySlot_Warn,
        SegmentDisplaySlot_Debug,
        SegmentDisplaySlot_Count,
    } segment_display_slot_t;

    typedef struct {
        char text[3];
        uint8_t len;
        bool active;
    } segment_display_slot_record_t;

// Variables:

    extern bool SegmentDisplay_NeedsUpdate;

// Functions:

    void SegmentDisplay_Update();
    void SegmentDisplay_SetText(uint8_t len, const char* text, segment_display_slot_t slot);
    void SegmentDisplay_SetInt(int32_t a, segment_display_slot_t slot);
    void SegmentDisplay_DeactivateSlot(segment_display_slot_t slot);
    void SegmentDisplay_UpdateKeymapText();
    bool SegmentDisplay_SlotIsActive(segment_display_slot_t slot);

#endif
#ifndef __SEGMENT_DISPLAY_H__
#define __SEGMENT_DISPLAY_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "macros/vars.h"

// Macros:

    #define SEGMENT_DISPLAY_TEXT_LENGTH 3

// Typedefs:

    typedef enum {
        SegmentDisplaySlot_Keymap,
        SegmentDisplaySlot_Macro,
        SegmentDisplaySlot_Error,
        SegmentDisplaySlot_Warn,
        SegmentDisplaySlot_Debug,
        SegmentDisplaySlot_Count,
    } segment_display_slot_t;

    typedef struct {
        char text[SEGMENT_DISPLAY_TEXT_LENGTH];
        uint8_t len;
        bool active;
    } segment_display_slot_record_t;

// Variables:

// Functions:

    void SegmentDisplay_Update();
    void SegmentDisplay_SetText(uint8_t len, const char* text, segment_display_slot_t slot);
    void SegmentDisplay_SetInt(int32_t a, segment_display_slot_t slot);
    void SegmentDisplay_SetFloat(float a, segment_display_slot_t slot);
    void SegmentDisplay_SerializeString(char* buffer, uint8_t bufLen, string_ref_t strRef);
    void SegmentDisplay_SerializeInt(char* buffer, uint8_t bufLen, int32_t a);
    void SegmentDisplay_SerializeFloat(char* buffer, uint8_t bufLen, float f);
    void SegmentDisplay_SerializeVar(char* buffer, uint8_t bufLen, macro_variable_t var);
    void SegmentDisplay_DeactivateSlot(segment_display_slot_t slot);
    void SegmentDisplay_UpdateKeymapText();
    bool SegmentDisplay_SlotIsActive(segment_display_slot_t slot);

#endif

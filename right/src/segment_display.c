#include "segment_display.h"
#include "event_scheduler.h"
#include "led_display.h"
#include "macros/core.h"
#include "macros/status_buffer.h"
#include "macros/vars.h"
#include "timer.h"
#include <string.h>
#include "keymap.h"
#include "utils.h"
#include "debug.h"


uint16_t changeInterval = 1500;
uint32_t lastChange = 0;
segment_display_slot_record_t slots[SegmentDisplaySlot_Count] = {
    [SegmentDisplaySlot_Keymap] = { .text = "   ", .active = true, .len = 3 }
};
uint8_t activeSlotCount = 1;
segment_display_slot_t currentSlot = SegmentDisplaySlot_Keymap;

bool SegmentDisplay_NeedsUpdate = false;

static void writeLedDisplay()
{
    LedDisplay_SetText(slots[currentSlot].len, slots[currentSlot].text);
}

static bool handleOverrides()
{
    if (slots[SegmentDisplaySlot_Debug].active) {
        currentSlot = SegmentDisplaySlot_Debug;
        return true;
    } else if (slots[SegmentDisplaySlot_Macro].active) {
        currentSlot = SegmentDisplaySlot_Macro;
        return true;
    } else {
        return false;
    }
}

static void changeSlot()
{
    if (!handleOverrides()) {
        do {
            currentSlot = (currentSlot + 1) % SegmentDisplaySlot_Count;
        } while (!slots[currentSlot].active);
    }
    lastChange = CurrentTime;
    if (activeSlotCount > 1) {
        EventScheduler_Reschedule(CurrentTime + changeInterval, EventSchedulerEvent_SegmentDisplayUpdate);
    }

    writeLedDisplay();
}

void SegmentDisplay_SetText(uint8_t len, const char* text, segment_display_slot_t slot)
{
    activeSlotCount += slots[slot].active ? 0 : 1;
    memcpy(&slots[slot].text, text, len);
    slots[slot].len = len;
    slots[slot].active = true;
    lastChange = CurrentTime;
    currentSlot = slot;
    handleOverrides();
    writeLedDisplay();
    if (activeSlotCount > 1) {
        EventScheduler_Reschedule(CurrentTime + changeInterval, EventSchedulerEvent_SegmentDisplayUpdate);
    }
}

void SegmentDisplay_DeactivateSlot(segment_display_slot_t slot)
{
    activeSlotCount -= slots[slot].active ? 1 : 0;
    slots[slot].active = false;
    if (currentSlot == slot) {
        changeSlot();
    }
}

void SegmentDisplay_Update()
{
    if (currentSlot == SegmentDisplaySlot_Debug) {
        activeSlotCount -= slots[SegmentDisplaySlot_Debug].active ? 1 : 0;
        slots[SegmentDisplaySlot_Debug].active = false;
    }
    if (CurrentTime - lastChange >= changeInterval) {
        changeSlot();
    }
}

void SegmentDisplay_UpdateKeymapText()
{
    keymap_reference_t *currentKeymap = AllKeymaps + CurrentKeymapIndex;
    SegmentDisplay_SetText(currentKeymap->abbreviationLen, currentKeymap->abbreviation, SegmentDisplaySlot_Keymap);
}

void SegmentDisplay_SerializeVar(char* buffer, macro_variable_t var)
{
    switch (var.type) {
        case MacroVariableType_Float:
            SegmentDisplay_SerializeFloat(buffer, var.asFloat);
            break;
        case MacroVariableType_Int:
            SegmentDisplay_SerializeInt(buffer, var.asInt);
            break;
        case MacroVariableType_Bool:
            SegmentDisplay_SerializeInt(buffer, var.asBool);
            break;
        default:
            Macros_ReportErrorNum("Unexpected variable type:", var.type, NULL);
            break;
    }
}

void SegmentDisplay_SerializeFloat(char* buffer, float num)
{
    if (num <= -10.0f || 10.0f <= num ) {
        SegmentDisplay_SerializeInt(buffer, num);
    }

    int mag = 0;
    bool negative = false;

    if (num < 0.0f) {
        num = -num;
        negative = true;
    }

    while (num < 10.0f) {
        mag++;
        num *= 10;
    }
    if (negative) {
        buffer[0] = 'Z' - mag + 1;
    } else {
        buffer[0] = 'A' + mag - 1;
    }
    buffer[1] = '0' + (uint8_t)(num / 10.0f);
    buffer[2] = '0' + (uint8_t)(num) % 10;
}

void SegmentDisplay_SerializeInt(char* buffer, int32_t a)
{
    int mag = 0;
    int num = a;
    bool negative = false;

    if (num < 0) {
        num = -num;
        negative = true;
    }

    if (num < 1000 && !negative) {
        buffer[0] = '0' + num / 100;
        buffer[1] = '0' + num % 100 / 10;
        buffer[2] = '0' + num % 10;
    } else {
        while (num >= 100) {
            mag++;
            num /= 10;
        }
        buffer[0] = '0' + num / 10;
        buffer[1] = '0' + num % 10;

        if (negative) {
            buffer[2] = mag == 0 ? '-' : ('Z' + 1 - mag);
        } else {
            buffer[2] = mag == 0 ? '0' : ('A' - 1 + mag);
        }
    }
}

void SegmentDisplay_SetInt(int32_t a, segment_display_slot_t slot)
{
    char b[3];
    SegmentDisplay_SerializeInt(b, a);
    SegmentDisplay_SetText(3, b, slot);
}

bool SegmentDisplay_SlotIsActive(segment_display_slot_t slot)
{
    return slots[slot].active;
}

#include "segment_display.h"
#include "event_scheduler.h"
#include "led_display.h"
#include "macros.h"
#include "timer.h"
#include <string.h>
#include "keymap.h"
#include "utils.h"
#include "debug.h"


uint16_t changeInterval = 2000;
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


void SegmentDisplay_SetInt(int32_t a, segment_display_slot_t slot)
{
    char b[3];
    int mag = 0;
    int num = a;
    if (num < 0) {
        SegmentDisplay_SetText(3, "NEG", slot);
    } else {
        if (num < 1000) {
            b[0] = '0' + num / 100;
            b[1] = '0' + num % 100 / 10;
            b[2] = '0' + num % 10;
        } else {
            while (num >= 100) {
                mag++;
                num /= 10;
            }
            b[0] = '0' + num / 10;
            b[1] = '0' + num % 10;
            b[2] = mag == 0 ? '0' : ('A' - 2 + mag);
        }
        SegmentDisplay_SetText(3, b, slot);
    }
}

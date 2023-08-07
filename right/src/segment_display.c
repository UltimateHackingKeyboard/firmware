#include "segment_display.h"
#include "event_scheduler.h"
#include "led_display.h"
#include "macros.h"
#include "timer.h"
#include <string.h>
#include "keymap.h"
#include "utils.h"


uint16_t changeInterval = 2000;
uint32_t lastChange = 0;
segment_display_slot_record_t slots[SegmentDisplaySlot_Count] = {
    [SegmentDisplaySlot_Keymap] = { .text = "   ", .active = true, .len = 3 }
};
uint8_t activeSlotCount = 1;
segment_display_slot_t currentSlot = SegmentDisplaySlot_Keymap;

bool SegmentDisplay_NeedsUpdate = false;

static bool overrideSlot() {
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

static void changeSlot() {
    if (!overrideSlot()) {
        do {
            currentSlot = (currentSlot + 1) % SegmentDisplaySlot_Count;
        } while (!slots[currentSlot].active);
    }
    lastChange = CurrentTime;
    if (activeSlotCount > 1) {
        EventScheduler_Schedule(CurrentTime + changeInterval, EventSchedulerEvent_SegmentDisplayUpdate);
    }
}

void SegmentDisplay_SetText(uint8_t len, const char* text, segment_display_slot_t slot)
{
    activeSlotCount += slots[slot].active ? 0 : 1;
    slots[slot].text = text;
    slots[slot].len = len;
    slots[slot].active = true;
    lastChange = CurrentTime;
    currentSlot = slot;
    overrideSlot();
    LedDisplay_SetText(slots[currentSlot].len, slots[currentSlot].text);
    if (activeSlotCount > 1) {
        EventScheduler_Schedule(CurrentTime + changeInterval, EventSchedulerEvent_SegmentDisplayUpdate);
    }
}

void SegmentDisplay_DeactivateSlot(segment_display_slot_t slot)
{
    activeSlotCount -= slots[slot].active ? 0 : 1;
    slots[slot].active = false;
    if (currentSlot == slot) {
        changeSlot();
    }
}

void SegmentDisplay_Update()
{
    if (CurrentTime - lastChange >= changeInterval) {
        changeSlot();
    }

    LedDisplay_SetText(slots[currentSlot].len, slots[currentSlot].text);
}

void SegmentDisplay_UpdateKeymapText()
{
    keymap_reference_t *currentKeymap = AllKeymaps + CurrentKeymapIndex;
    SegmentDisplay_SetText(currentKeymap->abbreviationLen, currentKeymap->abbreviation, SegmentDisplaySlot_Keymap);
}

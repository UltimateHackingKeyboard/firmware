#include <string.h>
#include "debug.h"
#ifndef __ZEPHYR__
#include "segment_display.h"
#endif

#ifdef WATCHES

#include "timer.h"
#include "key_states.h"
#include <limits.h>
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "macros/status_buffer.h"
#include "segment_display.h"

uint8_t CurrentWatch = 0;

static uint16_t tickCount = 0;
static uint32_t lastWatch = 0;
static uint32_t watchInterval = 500;

static void writeScancode(uint8_t b)
{
    Macros_SetStatusChar(' ');
    Macros_SetStatusNum(b);
}

void AddReportToStatusBuffer(char* dbgTag, usb_basic_keyboard_report_t *report)
{
    if (dbgTag != NULL && *dbgTag != '\0') {
        Macros_SetStatusString(dbgTag, NULL);
        Macros_SetStatusChar(' ');
    }
    Macros_SetStatusNum(report->modifiers);
    UsbBasicKeyboard_ForeachScancode(report, &writeScancode);
    Macros_SetStatusChar('\n');
}


void TriggerWatch(key_state_t *keyState)
{
    int16_t key = (keyState - &KeyStates[SlotId_LeftKeyboardHalf][0]);
    if (0 <= key && key <= 7) {
        // Set the LED value to RES until next update occurs.
        SegmentDisplay_SetText(3, "RES", SegmentDisplaySlot_Debug);
        CurrentWatch = key;
        tickCount = 0;
    }
}

void WatchTime(uint8_t n)
{
    static uint32_t lastUpdate = 0;
    if (CurrentTime - lastWatch > watchInterval) {
        SegmentDisplay_SetInt(CurrentTime - lastUpdate, SegmentDisplaySlot_Debug);
        lastWatch = CurrentTime;
    }
    lastUpdate = CurrentTime;
}

void WatchTimeMicros(uint8_t n)
{
    static uint32_t lastUpdate = 0;
    static uint16_t i = 0;

    i++;

    if (i == 1000) {
        SegmentDisplay_SetInt(CurrentTime - lastUpdate, SegmentDisplaySlot_Debug);
        lastUpdate = CurrentTime;
        i = 0;
    }
}


void WatchCallCount(uint8_t n)
{
    tickCount++;

    if (CurrentTime - lastWatch > watchInterval) {
        SegmentDisplay_SetInt(tickCount, SegmentDisplaySlot_Debug);
        lastWatch = CurrentTime;
    }
}

void WatchValue(int v, uint8_t n)
{
    if (CurrentTime - lastWatch > watchInterval) {
        SegmentDisplay_SetInt(v, SegmentDisplaySlot_Debug);
        lastWatch = CurrentTime;
    }
}

void WatchString(char const *v, uint8_t n)
{
    if (CurrentTime - lastWatch > watchInterval) {
        SegmentDisplay_SetText(strlen(v), v, SegmentDisplaySlot_Debug);
        lastWatch = CurrentTime;
    }
}

void ShowString(char const *v, uint8_t n)
{
    SegmentDisplay_SetText(strlen(v), v, SegmentDisplaySlot_Debug);
}

void ShowValue(int v, uint8_t n)
{
    SegmentDisplay_SetInt(v, SegmentDisplaySlot_Debug);
}


void WatchValueMin(int v, uint8_t n)
{
    static int m = 0;

    if (v < m) {
        m = v;
    }

    if (CurrentTime - lastWatch > watchInterval) {
        SegmentDisplay_SetInt(m, SegmentDisplaySlot_Debug);
        lastWatch = CurrentTime;
        m = INT_MAX;
    }
}

void WatchValueMax(int v, uint8_t n)
{
    static int m = 0;

    if (v > m) {
        m = v;
    }

    if (CurrentTime - lastWatch > watchInterval) {
        SegmentDisplay_SetInt(m, SegmentDisplaySlot_Debug);
        lastWatch = CurrentTime;
        m = INT_MIN;
    }
}


void WatchFloatValue(float v, uint8_t n)
{
    if (CurrentTime - lastWatch > watchInterval) {
        SegmentDisplay_SetFloat(v, SegmentDisplaySlot_Debug);
        lastWatch = CurrentTime;
    }
}

void WatchFloatValueMin(float v, uint8_t n)
{
    static float m = 0;

    if (v < m) {
        m = v;
    }

    if (CurrentTime - lastWatch > watchInterval) {
        SegmentDisplay_SetFloat(m, SegmentDisplaySlot_Debug);
        lastWatch = CurrentTime;
        m = (float)INT_MAX;
    }
}

void WatchFloatValueMax(float v, uint8_t n)
{
    static float m = 0;

    if (v > m) {
        m = v;
    }

    if (CurrentTime - lastWatch > watchInterval) {
        SegmentDisplay_SetFloat(m, SegmentDisplaySlot_Debug);
        lastWatch = CurrentTime;
        m = (float)INT_MIN;
    }
}


#endif

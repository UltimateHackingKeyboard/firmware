#include <string.h>
#include "debug.h"

#ifdef __ZEPHYR__
#include "logger.h"
#include "keyboard/oled/screens/screen_manager.h"
#include <zephyr/kernel.h>
#else
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

static void showInt(int32_t n) {
#ifdef __ZEPHYR__
    Log("%i: %i", CurrentWatch, n);
#else
    SegmentDisplay_SetInt(n, SegmentDisplaySlot_Debug);
#endif
}

static void showString(const char* str) {
#ifdef __ZEPHYR__
    Log("%i: %s", CurrentWatch, str);
#else
    SegmentDisplay_SetText(strlen(str), str, SegmentDisplaySlot_Debug);
#endif
}

static void showFloat(float f) {
#ifdef __ZEPHYR__
    Log("%i: %f", CurrentWatch, f);
#else
    SegmentDisplay_SetFloat(f, SegmentDisplaySlot_Debug);
#endif
}

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
        // Set the LED value to --- until next update occurs.
#ifdef __ZEPHYR__
        ScreenManager_ActivateScreen(ScreenId_Debug);
#endif
        showString("---");
        CurrentWatch = key;
        tickCount = 0;
    }
}

void WatchTime(uint8_t n)
{
    static uint32_t lastUpdate = 0;
    if (CurrentTime - lastWatch > watchInterval) {
        showInt(CurrentTime - lastUpdate);
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
        showInt(CurrentTime - lastUpdate);
        lastUpdate = CurrentTime;
        i = 0;
    }
}


void WatchCallCount(uint8_t n)
{
    tickCount++;

    if (CurrentTime - lastWatch > watchInterval) {
        showInt(tickCount);
        lastWatch = CurrentTime;
    }
}

void WatchValue(int v, uint8_t n)
{
    if (CurrentTime - lastWatch > watchInterval) {
        showInt(v);
        lastWatch = CurrentTime;
    }
}

void WatchString(char const *v, uint8_t n)
{
    if (CurrentTime - lastWatch > watchInterval) {
        showString(v);
        lastWatch = CurrentTime;
    }
}

void ShowString(char const *v, uint8_t n)
{
    showString(v);
}

void ShowValue(int v, uint8_t n)
{
    showInt(v);
}


void WatchValueMin(int v, uint8_t n)
{
    static int m = 0;

    if (v < m) {
        m = v;
    }

    if (CurrentTime - lastWatch > watchInterval) {
        showInt(m);
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
        showInt(m);
        lastWatch = CurrentTime;
        m = INT_MIN;
    }
}


void WatchFloatValue(float v, uint8_t n)
{
    if (CurrentTime - lastWatch > watchInterval) {
        showFloat(v);
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
        showFloat(m);
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
        showFloat(m);
        lastWatch = CurrentTime;
        m = (float)INT_MIN;
    }
}


#ifdef __ZEPHYR__
static const char* getJustFilename(const char* filename) {
    const char* p = filename;
    const char* lastSlash = filename;

    while (*p != '\0') {
        if (*p == '/') {
            lastSlash = p;
        }
        p++;
    }
    return ++lastSlash;
}

void WatchSemaforeTake(struct k_sem* sem, char const * label, uint8_t n) {
    if (k_sem_take(sem, K_NO_WAIT) != 0) {
        uint64_t startTimeUs = k_cyc_to_us_near64(k_cycle_get_32());
        k_sem_take(sem, K_FOREVER);
        uint64_t endTimeUs = k_cyc_to_us_near64(k_cycle_get_32());
        const char* threadName = k_thread_name_get(k_current_get());
        printk("Waited %lld us for semaphore %s in thread %s\n", endTimeUs - startTimeUs, getJustFilename(label), threadName);
    }
}
#endif // __ZEPHYR__
#else

#endif

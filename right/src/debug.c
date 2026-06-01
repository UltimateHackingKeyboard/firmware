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
#include "macros/status_buffer.h"
#include "hid/transport.h"
#include "segment_display.h"
#include "logger.h"

uint8_t CurrentWatch = 0;

static uint16_t tickCount = 0;
static uint32_t lastWatch = 0;

static void showInt(int32_t n) {
#ifdef __ZEPHYR__
    Log("W%i: %i\n", CurrentWatch, n);
#else
    SegmentDisplay_SetInt(n, SegmentDisplaySlot_Debug);
#endif
}

static void showString(const char* str) {
#ifdef __ZEPHYR__
    Log("W%i: %s\n", CurrentWatch, str);
#else
    SegmentDisplay_SetText(strlen(str), str, SegmentDisplaySlot_Debug);
#endif
}

static void showFloat(float f) {
#ifdef __ZEPHYR__
    uint16_t intPart = (uint16_t)f;
    uint16_t fracPart = (uint16_t)((f - intPart) * 100); // Show two decimal places
    Log("W%i: %u.%02u\n", CurrentWatch, intPart, fracPart);
#else
    SegmentDisplay_SetFloat(f, SegmentDisplaySlot_Debug);
#endif
}

static void writeScancode(uint8_t b)
{
    Macros_SetStatusChar(' ');
    Macros_SetStatusNum(b);
}

void AddReportToStatusBuffer(char* dbgTag, hid_keyboard_report_t *report)
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
    if (0 <= key && key <= 6) {
        // Set the LED value to --- until next update occurs.
#ifdef __ZEPHYR__
        if (DEBUG_CONSOLE) {
            ScreenManager_ActivateScreen(ScreenId_Debug);
        }
#endif
        showString("---");
        CurrentWatch = key;
        tickCount = 0;
    }
}

void WatchTime(uint8_t n)
{
    static uint32_t lastUpdate = 0;
    if (Timer_GetCurrentTime() - lastWatch > WATCH_INTERVAL) {
        showInt(Timer_GetCurrentTime() - lastUpdate);
        lastWatch = Timer_GetCurrentTime();
    }
    lastUpdate = Timer_GetCurrentTime();
}

bool WatchCondition(uint8_t n)
{
    if (Timer_GetCurrentTime() - lastWatch > WATCH_INTERVAL) {
        lastWatch = Timer_GetCurrentTime();
        return true;
    }
    return false;
}

void WatchTimeMicros(uint8_t n)
{
    static uint32_t lastUpdate = 0;
    static uint16_t i = 0;

    i++;

    if (i == 1000) {
        showInt(Timer_GetCurrentTime() - lastUpdate);
        lastUpdate = Timer_GetCurrentTime();
        i = 0;
    }
}


void WatchCallCount(uint8_t n)
{
    tickCount++;

    if (Timer_GetCurrentTime() - lastWatch > WATCH_INTERVAL) {
        showInt(tickCount);
        lastWatch = Timer_GetCurrentTime();
    }
}

void WatchValue(int v, uint8_t n)
{
    if (Timer_GetCurrentTime() - lastWatch > WATCH_INTERVAL) {
        showInt(v);
        lastWatch = Timer_GetCurrentTime();
    }
}

void WatchString(char const *v, uint8_t n)
{
    if (Timer_GetCurrentTime() - lastWatch > WATCH_INTERVAL) {
        showString(v);
        lastWatch = Timer_GetCurrentTime();
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

    if (Timer_GetCurrentTime() - lastWatch > WATCH_INTERVAL) {
        showInt(m);
        lastWatch = Timer_GetCurrentTime();
        m = INT_MAX;
    }
}

void WatchValueMax(int v, uint8_t n)
{
    static int m = 0;

    if (v > m) {
        m = v;
    }

    if (Timer_GetCurrentTime() - lastWatch > WATCH_INTERVAL) {
        showInt(m);
        lastWatch = Timer_GetCurrentTime();
        m = INT_MIN;
    }
}


void WatchFloatValue(float v, uint8_t n)
{
    if (Timer_GetCurrentTime() - lastWatch > WATCH_INTERVAL) {
        showFloat(v);
        lastWatch = Timer_GetCurrentTime();
    }
}

void WatchFloatValueMin(float v, uint8_t n)
{
    static float m = 0;

    if (v < m) {
        m = v;
    }

    if (Timer_GetCurrentTime() - lastWatch > WATCH_INTERVAL) {
        showFloat(m);
        lastWatch = Timer_GetCurrentTime();
        m = (float)INT_MAX;
    }
}

void WatchFloatValueMax(float v, uint8_t n)
{
    static float m = 0;

    if (v > m) {
        m = v;
    }

    if (Timer_GetCurrentTime() - lastWatch > WATCH_INTERVAL) {
        showFloat(m);
        lastWatch = Timer_GetCurrentTime();
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

/**
 * We observe whether a HID send was successful or not.
 *
 * If we compute reports too early we will see fails because of busy transports.
 * If we compute reports too late, we will see no fails, but we will see lower number of successes (/attempts) and higher latency.
 *
 * Latency is measured per transport as an average of the time between the report is dispatched and its "sent state" callback is called.
 * */
void Debug_RecordBleSendResult(int ret)
{
    if (DEBUG_BLE_LATENCY_STATS) {
        static uint32_t thisMs = 0;
        static uint32_t succ = 0;
        static uint32_t fail = 0;

        uint32_t now = Timer_GetCurrentTime();

        uint16_t latInt = (uint16_t)HidReportBleLatencyAvgMs;
        uint16_t latFra = (uint16_t)((HidReportBleLatencyAvgMs - latInt) * 100); // Show two decimal places

        if (now / 1024 != thisMs) {
            LogU("BLE report send: succ=%u, fail=%u, latency=%d.%d\n", succ, fail, latInt, latFra);

            thisMs = now / 1024;
            succ = 0;
            fail = 0;
        }

        if (ret == 0) {
            succ++;
        } else {
            fail++;
        }
    }
}

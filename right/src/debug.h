#ifndef __DEBUG_H__
#define __DEBUG_H__

#define WATCHES false
#define DEBUG_SEMAPHORES false
#define DEBUG_STATESYNC false
#define DEBUG_CONSOLE false
#define DEBUG_EVENTLOOP_SCHEDULE false
#define DEBUG_CHECK_MACRO_RUN_TIMES false
#define DEBUG_POSTPONER false
#define WATCH_INTERVAL 500

#define DEBUG_EVENTLOOP_TIMING false
#define DEBUG_THREAD_STATS true

#define DEBUG_MODE false
#define DEBUG_STRESS_UART false
#define DEBUG_STRESS_GATT false
#define DEBUG_STRESS_TRANSPORT false
#define DEBUG_TEST_RTT false
#define DEBUG_LOG_UART true
#define DEBUG_LOG_MESSAGES false

#define DEBUG_BATTERY_TESTING true
#define DEBUG_UHK60_SLEEPS false

#define DEBUG_ROLL_STATUS_BUFFER false

#define DEBUG_BLE_LATENCY_STATS false

#include <stdint.h>
#include <stdbool.h>
#include "timer.h"

// Last-seen timestamps of the key event pipeline stages; printed by Hid_DumpTransportState.
typedef struct {
    uint32_t scan;
    uint32_t queued;
    uint32_t forceQueued;
    uint32_t applied;
    uint32_t action;
    uint32_t delivered;
} key_life_times_t;

extern key_life_times_t KeyLifeTimes;

#define DEBUG_KEY_LIFE_ENABLED true
#if DEBUG_KEY_LIFE_ENABLED
#define DEBUG_KEY_LIFE(STAGE) (KeyLifeTimes.STAGE = Timer_GetCurrentTime())
#else
#define DEBUG_KEY_LIFE(STAGE)
#endif

// Last-seen state of blockedByReportThrottle(); printed by Hid_DumpTransportState.
typedef struct {
    uint32_t throttleTime;              // last blockedByReportThrottle() run
    uint32_t throttleBlockedUntil;
    uint32_t throttlePostponedMasks;
    uint8_t throttleBlockReasons;       // bitmask, see THROTTLE_REASON_*
    bool throttleBlocked;
} main_life_times_t;

#define THROTTLE_REASON_SEMAPHORE   (1 << 0)
#define THROTTLE_REASON_KEY_DELAY   (1 << 1)
#define THROTTLE_REASON_RETRY       (1 << 2)
#define THROTTLE_REASON_WINDOW      (1 << 3)

extern main_life_times_t MainLifeTimes;

// Last-seen start/end timestamps of every (uhk60) ISR; printed by Hid_DumpTransportState.
// A start newer than its end means the ISR is running or died mid-body.
typedef struct {
    uint32_t start;
    uint32_t end;
} isr_span_t;

typedef struct {
    isr_span_t usb;
    isr_span_t pitTimer;
    isr_span_t i2cWatchdog;
    isr_span_t i2cMain;
    isr_span_t resetButton;
} isr_life_times_t;

extern isr_life_times_t IsrLifeTimes;

#define ISR_LIFE_START(FIELD) (IsrLifeTimes.FIELD.start = Timer_GetCurrentTime())
#define ISR_LIFE_END(FIELD) (IsrLifeTimes.FIELD.end = Timer_GetCurrentTime())

#ifdef __ZEPHYR__
    #include "logger.h"
    #include <zephyr/kernel.h>
#endif

#include <stdint.h>
#include "key_states.h"
#include "hid/keyboard_report.h"
#include "trace_defs.h"

#if WATCHES

// When a key '~' to '6' is pressed, corresponding slot (identified by numbers 0-6) is activated.
// This means that corresponding watched value is shown on the LED display and then updated in certain intervals.
//
// Numbers are outputted in an exponent notation in form '[0-9][0-9]' + '[0A-Z]' where letter denotes added magnitude (10A = 1000, 10B = 10000...).
// Letters are used for magnitude so that brain is not confused by seeing three digit numbers.

    // This hook is to be placed in usb_report_updater and to be called whenever a key is activated (i.e., on key-down event).
    #define WATCH_TRIGGER(STATE) TriggerWatch(STATE);

    // Shows time between calls.
    #define WATCH_TIME(N) if(CurrentWatch == N) { WatchTime(N); }

    // Shows time between calls, averaged over 1000 calls.
    #define WATCH_TIME_MICROS(N) if(CurrentWatch == N) { WatchTimeMicros(N); }

    // Shows time between calls, averaged over 1000 calls.
    #define WATCH_CALL_COUNT(N) if(CurrentWatch == N) { WatchCallCount(N); }

    // Watches value V in slot N.
    #define WATCH_VALUE(V, N) if(CurrentWatch == N) { WatchValue(V, N); }

    // Watches value V in slot N.
    #define WATCH_VALUE_MIN(V, N) if(CurrentWatch == N) { WatchValueMin(V, N); }

    // Watches value V in slot N.
    #define WATCH_VALUE_MAX(V, N) if(CurrentWatch == N) { WatchValueMax(V, N); }

    // Watches value V in slot N.
    #define WATCH_FLOAT_VALUE(V, N) if(CurrentWatch == N) { WatchFloatValue(V, N); }

    // Watches value V in slot N.
    #define WATCH_FLOAT_VALUE_MIN(V, N) if(CurrentWatch == N) { WatchFloatValueMin(V, N); }

    // Watches value V in slot N.
    #define WATCH_FLOAT_VALUE_MAX(V, N) if(CurrentWatch == N) { WatchFloatValueMax(V, N); }

    // Watches string V in slot N.
    #define WATCH_STRING(V, N) if(CurrentWatch == N) { WatchString(V, N); }

    // Returns true if watch n should print
    #define WATCH_CONDITION(N) (CurrentWatch == N && WatchCondition(N))

    // Always show value, but respect slot logic
    #define SHOW_VALUE(V, N) if(CurrentWatch == N) { ShowValue(V, N); }

    // Always show string (no timing logic is applied), but respect slot logic.
    #define SHOW_STRING(V, N) if(CurrentWatch == N) { ShowString(V, N); }

    #define ERR(E) Macros_ReportError(E, NULL, NULL);

    #define ERRN(E, N) if(CurrentWatch == N) { Macros_ReportError(E, NULL, NULL); }

    #define ASSERT(C) if (!(C)) { Macros_ReportError("Assertion failed: "#C, NULL, NULL); }

    #define IF_DEBUG(CMD) CMD

    // Variables:

    extern uint8_t CurrentWatch;

    // Functions:

    void TriggerWatch(key_state_t *keyState);
    void WatchTime(uint8_t n);
    void WatchTimeMicros(uint8_t n);
    void WatchCallCount(uint8_t n);
    void WatchValue(int v, uint8_t n);
    void WatchValueMin(int v, uint8_t n);
    void WatchValueMax(int v, uint8_t n);
    void WatchFloatValue(float v, uint8_t n);
    void WatchFloatValueMin(float, uint8_t n);
    void WatchFloatValueMax(float, uint8_t n);
    void WatchString(char const * v, uint8_t n);
    bool WatchCondition(uint8_t n);
    void ShowValue(int v, uint8_t n);
    void ShowString(char const * v, uint8_t n);
    void AddReportToStatusBuffer(char* dbgTag, hid_keyboard_report_t *report);

    #ifdef __ZEPHYR__
        void WatchSemaforeTake(struct k_sem* sem, char const * label, uint8_t n);
    #endif


#else

    #define WATCH_TRIGGER(N)
    #define WATCH_TIME(N)
    #define WATCH_TIME_MICROS(N)
    #define WATCH_CALL_COUNT(N)
    #define WATCH_VALUE(V, N)
    #define WATCH_VALUE_MIN(V, N)
    #define WATCH_VALUE_MAX(V, N)
    #define WATCH_FLOAT_VALUE(V, N)
    #define WATCH_FLOAT_VALUE_MIN(V, N)
    #define WATCH_FLOAT_VALUE_MAX(V, N)
    #define WATCH_STRING(V, N)
    #define WATCH_CONDITION(N) false
    #define SHOW_STRING(V, N)
    #define SHOW_VALUE(V, N)
    #define ERR(E)
    #define ERRN(E, N)
    #define ASSERT(C)
    #define IF_DEBUG(CMD)

#endif

#if defined(__ZEPHYR__) && DEBUG_SEMAPHORES
#define WATCH_SEMAPHORE_TAKE(SEM, FILENAME, N) if(CurrentWatch == N) { WatchSemaforeTake(SEM, FILENAME, N); } else { k_sem_take(SEM, K_FOREVER); }
#define SEM_TAKE(SEM) WATCH_SEMAPHORE_TAKE(SEM, __FILE__, 0)
#else
#define SEM_TAKE(SEM) if (k_sem_take(SEM, K_MSEC(256)) != 0) { uint8_t tgt = Cfg.DevMode ? LogTarget_Uart | LogTarget_ErrorBuffer : LogTarget_Uart; LogTo(DEVICE_ID, tgt, "Failed to take semaphore " #SEM " in file %s.!\n", __FILE__); Trace_Print(tgt, "Failed semaphore");  }
#define WATCH_SEMAPHORE_TAKE(SEM, LABEL, N) k_sem_take(SEM, K_FOREVER);
#endif

#if defined(__ZEPHYR__) && DEBUG_STATESYNC
#define STATE_SYNC_LOG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define STATE_SYNC_LOG(fmt, ...)
#endif

#if defined(__ZEPHYR__) && DEBUG_EVENTLOOP_SCHEDULE
#define LOG_SCHEDULE(CODE) CODE
#else
#define LOG_SCHEDULE(CODE)
#endif

#if defined(__ZEPHYR__) && DEBUG_POSTPONER
#define LOG_POSTPONER(CODE) CODE
#else
#define LOG_POSTPONER(CODE)
#endif

#if defined(__ZEPHYR__) && DEBUG_EVENTLOOP_TIMING
#define EVENTLOOP_TIMING(CODE) CODE
#else
#define EVENTLOOP_TIMING(CODE)
#endif

void Debug_RecordBleSendResult(int ret);

#ifndef __ZEPHYR__
    // Fill the free main/MSP stack (and the unused gap below it) with a canary
    // pattern; call as the first thing in main.
    void Debug_InitStackCanary(void);
    // Size of the MSP stack region (__StackTop - __StackLimit).
    uint32_t Debug_StackSize(void);
    // All-time peak stack usage of main + ISRs combined.
    uint32_t Debug_StackUsed(void);
    // Unused bytes below the peak; negative = overflowed past __StackLimit by that much.
    int32_t Debug_StackHeadroom(void);
#endif

#endif // __DEBUG_H__

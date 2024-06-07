#define WATCHES false
#define DEBUG_SEMAPHORES false
#define DEBUG_EVENTLOOP_TIMING false

#if WATCHES && !defined(SRC_UTILS_DBG_H_)

// When a key '~' to '6' is pressed, corresponding slot (identified by numbers 0-6) is activated.
// This means that corresponding watched value is shown on the LED display and then updated in certain intervals.
//
// Numbers are outputted in an exponent notation in form '[0-9][0-9]' + '[0A-Z]' where letter denotes added magnitude (10A = 1000, 10B = 10000...).
// Letters are used for magnitude so that brain is not confused by seeing three digit numbers.

// Includes:

    #ifdef __ZEPHYR__
    #include "logger.h"
    #include <zephyr/kernel.h>
    #endif

    #include <stdint.h>
    #include "key_states.h"
    #include "usb_interfaces/usb_interface_basic_keyboard.h"

// Macros:

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

    // Always show value, but respect slot logic
    #define SHOW_VALUE(V, N) if(CurrentWatch == N) { ShowValue(V, N); }

    // Always show string (no timing logic is applied), but respect slot logic.
    #define SHOW_STRING(V, N) if(CurrentWatch == N) { ShowString(V, N); }

    #define ERR(E) Macros_ReportError(E, NULL, NULL);

    #define ERRN(E, N) if(CurrentWatch == N) { Macros_ReportError(E, NULL, NULL); }

    #define ASSERT(C) if (!(C)) { Macros_ReportError("Assertion failed: "#C, NULL, NULL); }

    #define IF_DEBUG(CMD) CMD

    #ifdef __ZEPHYR__

        #define WATCH_SEMAPHORE_TAKE(SEM, FILENAME, N) if(CurrentWatch == N) { WatchSemaforeTake(SEM, FILENAME, N); } else { k_sem_take(SEM, K_FOREVER); }

        #if DEBUG_SEMAPHORES
        #define SEM_TAKE(SEM) WATCH_SEMAPHORE_TAKE(SEM, __FILE__, 0)
        #else
        #define SEM_TAKE(SEM) k_sem_take(SEM, K_FOREVER)
        #endif

    #endif


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
    void ShowValue(int v, uint8_t n);
    void ShowString(char const * v, uint8_t n);
    void AddReportToStatusBuffer(char* dbgTag, usb_basic_keyboard_report_t *report);

    #ifdef __ZEPHYR__
        void WatchSemaforeTake(struct k_sem* sem, char const * label, uint8_t n);
    #endif


#else

// Macros:

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
    #define WATCH_SEMAPHORE_TAKE(SEM, LABEL, N) k_sem_take(SEM, K_FOREVER);
    #define SEM_TAKE(SEM) k_sem_take(SEM, K_FOREVER);
    #define SHOW_STRING(V, N)
    #define SHOW_VALUE(V, N)
    #define ERR(E)
    #define ERRN(E, N)
    #define ASSERT(C)
    #define IF_DEBUG(CMD)

#endif

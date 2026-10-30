#ifndef __LOGGER_H__
#define __LOGGER_H__

// Includes:

    #include "device.h"
    #include "stdbool.h"
    #include "stdint.h"
    #include "timer.h"

// Macros:

    #define WATCH_LOG_INTERVAL_MS 1000

    // Limits a log statement to one per WATCH_LOG_INTERVAL_MS, per call site.
    #define WATCH(...) \
        do { \
            static bool watchStarted = false; \
            static uint32_t watchLastTime = 0; \
            static uint32_t watchSuppressed = 0; \
            uint32_t watchNow = Timer_GetCurrentTime(); \
            if (!watchStarted || watchNow - watchLastTime >= WATCH_LOG_INTERVAL_MS) { \
                bool watchIsRecent = watchNow - watchLastTime < 2 * WATCH_LOG_INTERVAL_MS; \
                watchStarted = true; \
                watchLastTime = watchNow; \
                __VA_ARGS__; \
                if (watchSuppressed > 0 && watchIsRecent) { \
                    LogU("(%u similar messages suppressed)\n", (unsigned int)watchSuppressed); \
                } \
                watchSuppressed = 0; \
            } else { \
                watchSuppressed++; \
            } \
        } while (0)

// Variables:

typedef enum {
    LogTarget_Oled = 1,
    LogTarget_Uart = 2,
    LogTarget_ErrorBuffer = 4,
} log_target_t;

// Functions:

#if DEVICE_HAS_OLED
    void Oled_LogConstant(const char* text);
    void Oled_Log(const char *fmt, ...);
#endif

#ifdef __ZEPHYR__
    void Uart_LogConstant(const char* buffer);
    void Uart_Log(const char *fmt, ...);
    void InitLogLevels();
#endif

    // logging hook injected into the patched c2usb
    void c2usb_log(const char *fmt, ...);

    void Log(const char *fmt, ...);
    void LogTo(device_id_t deviceId, log_target_t logMask, const char *fmt, ...);
    void LogConstantTo(device_id_t deviceId, log_target_t logMask, const char* buffer);

    // Log to UART and OLED and State buffer
    void LogU(const char *fmt, ...);
    void LogUS(const char *fmt, ...);
    void LogUO(const char *fmt, ...);
    void LogO(const char *fmt, ...);
    void LogS(const char *fmt, ...);
    void LogUOS(const char *fmt, ...);
    void LogUSDO(const char *fmt, ...);


    void LogWrn(const char *fmt, ...);
    void LogErr(const char *fmt, ...);
    void LogInf(const char *fmt, ...);
    void LogDbg(const char *fmt, ...);

#ifndef __ZEPHYR__
#define LOG_WRN(fmt, ...) LogWrn(fmt"\n", ##__VA_ARGS__)
#define LOG_ERR(fmt, ...) LogErr(fmt"\n", ##__VA_ARGS__)
#define LOG_INF(fmt, ...) LogInf(fmt"\n", ##__VA_ARGS__)
#endif

#endif // __LOGGER_H__

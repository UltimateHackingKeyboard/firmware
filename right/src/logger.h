#ifndef __LOGGER_H__
#define __LOGGER_H__

// Includes:

    #include "device.h"
    #include "stdint.h"

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
#define LOG_WRN(fmt, ...) LogWrn(fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...) LogErr(fmt, ##__VA_ARGS__)
#define LOG_INF(fmt, ...) LogInf(fmt, ##__VA_ARGS__)
#endif

#endif // __LOGGER_H__

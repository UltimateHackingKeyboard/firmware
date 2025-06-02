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
#endif

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

#endif // __LOGGER_H__

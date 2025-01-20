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

    void Oled_LogConstant(const char* text);
    void Oled_Log(const char *fmt, ...);
    void Uart_LogConstant(const char* buffer);
    void Uart_Log(const char *fmt, ...);
    void Log(const char *fmt, ...);
    void LogTo(device_id_t deviceId, log_target_t logMask, const char *fmt, ...);
    void LogConstantTo(device_id_t deviceId, log_target_t logMask, const char* buffer);

    // Log to UART and OLED and State buffer
    void LogUO(const char *fmt, ...);
    void LogUOS(const char *fmt, ...);

#endif // __LOGGER_H__

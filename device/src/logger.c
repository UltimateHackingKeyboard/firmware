#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "logger.h"
#include "shell.h"
#include "keyboard/uart.h"
#include "nus_client.h"
#include "nus_server.h"
#include "device.h"
#include "messenger.h"
#include "macros/status_buffer.h"
#include "zephyr/device.h"

#ifdef DEVICE_HAS_OLED
#include "keyboard/oled/widgets/console_widget.h"
#endif

#define MAX_LOG_LENGTH 64

#define EXPAND_STRING(BUFFER)  \
char BUFFER[MAX_LOG_LENGTH]; \
{ \
    va_list myargs; \
    va_start(myargs, fmt); \
    BUFFER[MAX_LOG_LENGTH-1] = '\0'; \
    vsnprintf(BUFFER, MAX_LOG_LENGTH-1, fmt, myargs); \
}

void Uart_LogConstant(const char* buffer) {
    printk("%s", buffer);
}

void Uart_Log(const char *fmt, ...) {
    EXPAND_STRING(buffer);

    Uart_LogConstant(buffer);
}

void Log(const char *fmt, ...) {
    EXPAND_STRING(buffer);

    LogConstantTo(DEVICE_ID, LogTarget_Uart, buffer);
}

void LogU(const char *fmt, ...) {
    EXPAND_STRING(buffer);

    LogConstantTo(DEVICE_ID, LogTarget_Uart, buffer);
}

void LogUO(const char *fmt, ...) {
    EXPAND_STRING(buffer);

    LogConstantTo(DEVICE_ID, LogTarget_Uart | LogTarget_Oled, buffer);
}

void LogUOS(const char *fmt, ...) {
    EXPAND_STRING(buffer);

    LogConstantTo(DEVICE_ID, LogTarget_Uart | LogTarget_Oled | LogTarget_ErrorBuffer, buffer);
}

void LogConstantTo(device_id_t deviceId, log_target_t logMask, const char* buffer) {
    if (DEVICE_ID == deviceId) {
        if (logMask & LogTarget_Oled) {
            Oled_LogConstant(buffer);
        }
        if (logMask & LogTarget_Uart) {
            Uart_LogConstant(buffer);
        }
        if (logMask & LogTarget_ErrorBuffer) {
            Macros_ReportPrintf(NULL, "%s", buffer);
        }
    } else {
        Messenger_Send2(deviceId, MessageId_Log, logMask, buffer, strlen(buffer)+1);
    }
}

void LogTo(device_id_t deviceId, log_target_t logMask, const char *fmt, ...) {
    EXPAND_STRING(buffer);

    LogConstantTo(deviceId, logMask, buffer);
}


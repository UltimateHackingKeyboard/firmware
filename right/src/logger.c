#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "logger.h"
#include "device.h"
#include "macros/status_buffer.h"
#include "macro_events.h"
#include "debug.h"


#ifdef __ZEPHYR__
    #include "shell.h"
    #include "nus_client.h"
    #include "nus_server.h"
    #include "messenger.h"
    #include "zephyr/device.h"
    #include <zephyr/kernel.h>
    #include <zephyr/drivers/gpio.h>
    #include <zephyr/arch/arch_interface.h>
    #if DEVICE_IS_KEYBOARD
        #include "keyboard/uart_bridge.h"
        #ifdef DEVICE_HAS_OLED
            #include "keyboard/oled/widgets/console_widget.h"
        #endif
    #endif
#endif


#define MAX_LOG_LENGTH 256

#define EXPAND_STRING(BUFFER)  \
char BUFFER[MAX_LOG_LENGTH]; \
{ \
    va_list myargs; \
    va_start(myargs, fmt); \
    BUFFER[MAX_LOG_LENGTH-1] = '\0'; \
    vsnprintf(BUFFER, MAX_LOG_LENGTH-1, fmt, myargs); \
}

void Uart_LogConstant(const char* buffer) {
#ifdef __ZEPHYR__
    printk("%s", buffer);
#endif
}

void Uart_Log(const char *fmt, ...) {
#ifdef __ZEPHYR__
    EXPAND_STRING(buffer);

    Uart_LogConstant(buffer);
#endif
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

void LogUS(const char *fmt, ...) {
    EXPAND_STRING(buffer);

    LogConstantTo(DEVICE_ID, LogTarget_Uart | LogTarget_ErrorBuffer, buffer);
}

void LogS(const char *fmt, ...) {
    EXPAND_STRING(buffer);

    LogConstantTo(DEVICE_ID, LogTarget_ErrorBuffer, buffer);
}

void LogO(const char *fmt, ...) {
    EXPAND_STRING(buffer);

    LogConstantTo(DEVICE_ID, LogTarget_Oled, buffer);
}

void LogUOS(const char *fmt, ...) {
    EXPAND_STRING(buffer);

    LogConstantTo(DEVICE_ID, LogTarget_Uart | LogTarget_Oled | LogTarget_ErrorBuffer, buffer);
    MacroEvent_OnError();
}

void LogUSDO(const char *fmt, ...) {
    EXPAND_STRING(buffer);

    uint32_t logMask;

    if (DEBUG_MODE) {
        logMask = LogTarget_Uart | LogTarget_Oled | LogTarget_ErrorBuffer;
    } else {
        logMask = LogTarget_Uart | LogTarget_ErrorBuffer;
    }

    LogConstantTo(DEVICE_ID, logMask, buffer);
    MacroEvent_OnError();
}

void LogConstantTo(device_id_t deviceId, log_target_t logMask, const char* buffer) {
    if (DEVICE_IS_UHK60 || DEVICE_ID == deviceId) {
#if DEVICE_HAS_OLED
        if (logMask & LogTarget_Oled) {
            Oled_LogConstant(buffer);
        }
#endif
#ifdef __ZEPHYR__
        if ((logMask & LogTarget_Uart) && DEBUG_LOG_UART) {
            Uart_LogConstant(buffer);
        }
#endif
        if (logMask & LogTarget_ErrorBuffer) {
            Macros_PrintfWithPos(NULL, "%s", buffer);
        }
    } else {
#ifdef __ZEPHYR__
        if (k_is_in_isr()) {
            printk("Cannot send log from ISR:\n    %s\n", buffer);
        } else {
            Messenger_Send2(deviceId, MessageId_Log, logMask, buffer, strlen(buffer)+1);
        }
#endif
    }
}

void LogTo(device_id_t deviceId, log_target_t logMask, const char *fmt, ...) {
    EXPAND_STRING(buffer);

    LogConstantTo(deviceId, logMask, buffer);
}


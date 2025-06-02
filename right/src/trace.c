#include "trace.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "macros/status_buffer.h"
#include "wormhole.h"
#include "event_scheduler.h"
#include "logger.h"
#include "versioning.h"

#ifdef __ZEPHYR__
#include "proxy_log_backend.h"
#else
#define ProxyLog_IsInPanicMode false
#endif

static bool enabled = true;

#define EXPAND_STRING(BUFFER, MAX_LOG_LENGTH)  \
char BUFFER[MAX_LOG_LENGTH]; \
{ \
    va_list myargs; \
    va_start(myargs, fmt); \
    BUFFER[MAX_LOG_LENGTH-1] = '\0'; \
    vsnprintf(BUFFER, MAX_LOG_LENGTH-1, fmt, myargs); \
}

#define TraceBuffer StateWormhole.traceBuffer.data
#define TraceBufferPosition StateWormhole.traceBuffer.position

void Trace(char a) {
    if (enabled) {
        StateWormhole.traceBuffer.eventVector = EventScheduler_Vector;
        TraceBuffer[TraceBufferPosition] = a;
        TraceBufferPosition = (TraceBufferPosition + 1) % TRACE_BUFFER_SIZE;
    }
}

void Trace_Printf(const char *fmt, ...) {
    if (enabled) {
        EXPAND_STRING(buffer, TRACE_BUFFER_SIZE);

        for (uint16_t i = 0; i < TRACE_BUFFER_SIZE; i++) {
            if (buffer[i] == '\0' || buffer[i] > 126) {
                break;
            }
            if (buffer[i] == '\n') {
                Trace(' ');
                continue;
            }
            Trace(buffer[i]);
        }
    }
}

void Trace_Printc(const char* s) {
    if (enabled) {
        for (uint16_t i = 0; s[i] != '\0'; i++) {
            if (s[i] == '\0' || s[i] > 126) {
                break;
            }
            if (s[i] == '\n') {
                Trace(' ');
                continue;
            }
            Trace(s[i]);
        }
    }
}

void Trace_Init(void) {
    for (uint16_t i = 0; i < TRACE_BUFFER_SIZE; i++) {
        if (TraceBuffer[i] < 32 || TraceBuffer[i] > 126) {
            TraceBuffer[i] = ' ';
        }
    }
    if (TraceBufferPosition >= TRACE_BUFFER_SIZE) {
        TraceBufferPosition = 0;
    }
    Trace_Printc("###");
}

void Trace_Print(const char* reason) {
    uint16_t iter;
    enabled = false;

    device_id_t targetDeviceId = DEVICE_ID;
    log_target_t targetInterface = 0;

    if (ProxyLog_IsInPanicMode) {
        targetInterface = LogTarget_Uart;
    } else {
        targetInterface = LogTarget_Uart | LogTarget_ErrorBuffer;
    }

    LogTo(targetDeviceId, targetInterface, "Printing trace buffer because: %s\n", reason);
    LogTo(targetDeviceId, targetInterface, "EV: %d\n", StateWormhole.traceBuffer.eventVector);
    LogTo(targetDeviceId, targetInterface, "Tag: %s\n", gitTag);
    LogTo(targetDeviceId, targetInterface, "Trace:\n");

#define LINE_LENGTH 64

    char buff[LINE_LENGTH+1];

    for (iter = 0; iter < TRACE_BUFFER_SIZE; iter++) {
        char c = TraceBuffer[(TraceBufferPosition+iter)%TRACE_BUFFER_SIZE];
        if (c < 32) {
            buff[iter%LINE_LENGTH] = '?';
        } else if (c < 127) {
            buff[iter%LINE_LENGTH] = c;
        } else {
            buff[iter%LINE_LENGTH] = '?';
        }
        if ((iter+1) % 64 == 0) {
            buff[LINE_LENGTH] = '\0';
            LogTo(targetDeviceId, targetInterface, "%s\n", buff);
        }
    }

    enabled = true;
#undef LINE_LENGTH
}


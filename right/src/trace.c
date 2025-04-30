#include "trace.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "macros/status_buffer.h"
#include "wormhole.h"
#include "event_scheduler.h"

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

void Trace_Init(void) {
    for (uint16_t i = 0; i < TRACE_BUFFER_SIZE; i++) {
        if (TraceBuffer[i] < 32 || TraceBuffer[i] > 126) {
            TraceBuffer[i] = ' ';
        }
    }
    if (TraceBufferPosition >= TRACE_BUFFER_SIZE) {
        TraceBufferPosition = 0;
    }
    Trace_Printf("###");
}

void Trace_Print(const char* reason) {
    uint16_t iter;
    enabled = false;

    Macros_ReportPrintf("Printing trace buffer because: %s\n", reason);
    Macros_ReportPrintf("Last EV: %d\n", StateWormhole.traceBuffer.eventVector);
    Macros_ReportPrintf("Trace:\n");

    for (iter = 0; iter < TRACE_BUFFER_SIZE; iter++) {
        char c = TraceBuffer[(TraceBufferPosition+iter)%TRACE_BUFFER_SIZE];
        if (c < 32) {
            Macros_SetStatusChar('.');
        } else if (c <= 126) {
            Macros_SetStatusChar(c);
        } else {
            Macros_SetStatusChar('.');
        }
        if ((iter+1) % 64 == 0) {
            Macros_SetStatusChar('\n');
        }
    }
    Macros_SetStatusChar('\n');

    enabled = true;
}


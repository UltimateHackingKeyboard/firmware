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
            if (buffer[i] == '\0') {
                break;
            }
            if (buffer[i] == '\n') {
                Trace(' ');
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

void Trace_Print(void) {
    uint16_t iter;
    enabled = false;

    Macros_ReportPrintf("Last EV: %d\n", StateWormhole.traceBuffer.eventVector);
    Macros_ReportPrintf("Trace:");
    const uint16_t sliceLength = 64;

    uint16_t remains = 0;
    iter = TraceBufferPosition;
    while (iter < TRACE_BUFFER_SIZE) {
        uint16_t end = MIN(TRACE_BUFFER_SIZE, iter + sliceLength);
        remains = iter+sliceLength - TRACE_BUFFER_SIZE;
        Macros_ReportPrintf("\n%.*s", end-iter, TraceBuffer + iter);
        iter = end;
    }
    Macros_ReportPrintf("%.*s\n", remains, TraceBuffer);
    iter = remains;
    while (iter < TraceBufferPosition) {
        uint16_t end = MIN(TraceBufferPosition, iter + sliceLength);
        Macros_ReportPrintf("%.*s\n", end-iter, TraceBuffer + iter);
        iter = end;
    }

    enabled = true;
}


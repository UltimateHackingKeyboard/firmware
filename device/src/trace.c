#include "trace.h"
#include "logger.h"
#include <stdio.h>

#define TRACE_BUFFER_SIZE 512

static bool enabled = true;

#define EXPAND_STRING(BUFFER, MAX_LOG_LENGTH)  \
char BUFFER[MAX_LOG_LENGTH]; \
{ \
    va_list myargs; \
    va_start(myargs, fmt); \
    BUFFER[MAX_LOG_LENGTH-1] = '\0'; \
    vsnprintf(BUFFER, MAX_LOG_LENGTH-1, fmt, myargs); \
}


char traceBuffer[TRACE_BUFFER_SIZE];
uint16_t traceBufferPosition = 0;

void Trace(char a) {
    if (enabled) {
        traceBuffer[traceBufferPosition] = a;
        traceBufferPosition = (traceBufferPosition + 1) % TRACE_BUFFER_SIZE;
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
        traceBuffer[i] = ' ';
    }
    traceBufferPosition = 0;
}

void Trace_Print(void) {
    uint16_t iter;
    enabled = false;

    LogUS("Trace:");
    const uint16_t sliceLength = 64;

    uint16_t remains = 0;
    iter = traceBufferPosition;
    while (iter < TRACE_BUFFER_SIZE) {
        uint16_t end = MIN(TRACE_BUFFER_SIZE, iter + sliceLength);
        remains = iter+sliceLength - TRACE_BUFFER_SIZE;
        LogUS("\n%.*s", end-iter, traceBuffer + iter);
        iter = end;
    }
    LogUS("%.*s\n", remains, traceBuffer);
    iter = remains;
    while (iter < traceBufferPosition) {
        uint16_t end = MIN(traceBufferPosition, iter + sliceLength);
        LogUS("%.*s\n", end-iter, traceBuffer + iter);
        iter = end;
    }

    enabled = true;
}


#include "trace.h"
#include "logger.h"
#include <stdio.h>

#define TRACE_BUFFER_SIZE 128


#define EXPAND_STRING(BUFFER, MAX_LOG_LENGTH)  \
char BUFFER[MAX_LOG_LENGTH]; \
{ \
    va_list myargs; \
    va_start(myargs, fmt); \
    BUFFER[MAX_LOG_LENGTH-1] = '\0'; \
    vsnprintf(BUFFER, MAX_LOG_LENGTH-1, fmt, myargs); \
}


char traceBuffer[TRACE_BUFFER_SIZE];
uint8_t traceBufferPosition = 0;

void Trace(char a) {
    traceBuffer[traceBufferPosition] = a;
    traceBufferPosition = (traceBufferPosition + 1) % TRACE_BUFFER_SIZE;
}

void Trace_Printf(const char *fmt, ...) {
    EXPAND_STRING(buffer, TRACE_BUFFER_SIZE);

    for (uint8_t i = 0; i < TRACE_BUFFER_SIZE; i++) {
        if (buffer[i] == '\0') {
            break;
        }
        if (buffer[i] == '\n') {
            Trace(' ');
        }
        Trace(buffer[i]);
    }
}

void Trace_Init(void) {
    for (uint8_t i = 0; i < TRACE_BUFFER_SIZE; i++) {
        traceBuffer[i] = ' ';
    }
    traceBufferPosition = 0;
}

void Trace_Print(void) {
    LogUS("Trace: %.*s%.*s", TRACE_BUFFER_SIZE - traceBufferPosition, traceBuffer + traceBufferPosition, traceBufferPosition, traceBuffer);
}


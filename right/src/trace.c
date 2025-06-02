#include "trace.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "macros/status_buffer.h"
#include "wormhole.h"
#include "event_scheduler.h"
#include "logger.h"
#include "versioning.h"

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

#define LOG_TO LogU

    LOG_TO("Printing trace buffer because: %s\n", reason);
    LOG_TO("EV: %d\n", StateWormhole.traceBuffer.eventVector);
    LOG_TO("Tag: %s\n", gitTag);
    LOG_TO("Trace:\n");

#define LINE_LENGTH 64

    char buff[LINE_LENGTH+1];

    for (iter = 0; iter < TRACE_BUFFER_SIZE; iter++) {
        char c = TraceBuffer[(TraceBufferPosition+iter)%TRACE_BUFFER_SIZE];
        if (c < 32) {
            buff[iter%LINE_LENGTH] = '.';
        } else if (c < 127) {
            buff[iter%LINE_LENGTH] = c;
        } else {
            buff[iter%LINE_LENGTH] = '.';
        }
        if ((iter+1) % 64 == 0) {
            buff[LINE_LENGTH] = '\0';
            LOG_TO("%s\n", buff);
        }
    }

    enabled = true;
#undef LINE_LENGTH
#undef LOG_TO
}


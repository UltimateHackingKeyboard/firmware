#ifndef __TRACE_H__
#define __TRACE_H__

// Includes:

    #include <stdbool.h>
    #include <stdarg.h>
    #include <stdio.h>
    #include "trace_defs.h"
    #include "logger.h"
    #include "wormhole.h"
    #include "event_scheduler.h"

// Variables:

    extern bool Trace_Enabled;

// Macros:

    #define TraceBuffer StateWormhole.traceBuffer.data
    #define TraceBufferPosition StateWormhole.traceBuffer.position

// Inline functions:

    static inline void Trace(char a) {
        if (Trace_Enabled) {
            StateWormhole.traceBuffer.eventVector = EventScheduler_Vector;
            TraceBuffer[TraceBufferPosition] = a;
            TraceBufferPosition = (TraceBufferPosition + 1) % TRACE_BUFFER_SIZE;
        }
    }

    static inline void Trace_Printc(const char* s) {
        if (Trace_Enabled) {
            StateWormhole.traceBuffer.eventVector = EventScheduler_Vector;
            for (uint16_t i = 0; s[i] != '\0' && s[i] < 127; i++) {
                TraceBuffer[TraceBufferPosition] = s[i];
                TraceBufferPosition = (TraceBufferPosition + 1) % TRACE_BUFFER_SIZE;
            }
        }
    }

    static inline void Trace_Printf(const char *fmt, ...) {
        if (Trace_Enabled) {
            char buffer[TRACE_BUFFER_SIZE];
            va_list args;
            va_start(args, fmt);
            buffer[TRACE_BUFFER_SIZE-1] = '\0';
            vsnprintf(buffer, TRACE_BUFFER_SIZE-1, fmt, args);
            va_end(args);
            Trace_Printc(buffer);
        }
    }

// Functions:

    void Trace_Init(void);
    void Trace_Print(log_target_t additionalLogTargets, const char* reason);

#endif


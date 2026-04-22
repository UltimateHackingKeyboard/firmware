#include "trace.h"
#include <string.h>
#include "macros/status_buffer.h"
#include "trace_reasons.h"
#include "logger.h"
#include "versioning.h"

#include "device.h"

#ifdef __ZEPHYR__
#include "shell/sinks.h"
#else
#define ShellConfig_IsInPanicMode false
#endif

bool Trace_Enabled = true;

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

void Trace_Print(log_target_t additionalLogTargets, const char* reason) {
    uint16_t iter;
    Trace_Enabled = false;

    device_id_t targetDeviceId = DEVICE_ID;
    log_target_t targetInterface = 0;

    if (ShellConfig_IsInPanicMode) {
        targetInterface = LogTarget_Uart;
    } else {
        targetInterface = LogTarget_Uart | additionalLogTargets;
    }

    LogTo(targetDeviceId, targetInterface, "Printing trace buffer because: %s\n", reason);
    LogTo(targetDeviceId, targetInterface, "ID: %d, EV: %d, Tag: %s\n", DEVICE_ID, StateWormhole.traceBuffer.eventVector, gitTag);

#ifndef __ZEPHYR__
    Trace_PrintUhk60ReasonRegisters(targetDeviceId, targetInterface);
#endif

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

    Trace_Enabled = true;
#undef LINE_LENGTH
}


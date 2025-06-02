#include "proxy_log_backend.h"
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_backend.h>
#include <stdbool.h>
#include <stdint.h>
#include "macros/status_buffer.h"
#include "trace.h"
#include "wormhole.h"

#define PROXY_BACKEND_BUFFER_SIZE 2048

static bool isInPanicMode = false;

static char buffer[PROXY_BACKEND_BUFFER_SIZE];
uint16_t bufferPosition = 0;
uint16_t bufferLength = 0;

bool ProxyLog_HasLog = false;
bool ProxyLog_IsAttached = false;

#define POS(x) ((bufferPosition + (x) + PROXY_BACKEND_BUFFER_SIZE) % PROXY_BACKEND_BUFFER_SIZE)

static void updateNonemptyFlag() {
    ProxyLog_HasLog = (bufferLength > 0);
}

uint16_t ProxyLog_ConsumeLog(uint8_t* outBuf, uint16_t outBufSize) {
    uint16_t copied = 0;
    uint16_t remaining = MIN(bufferLength, outBufSize);
    while (remaining > 0) {
        char a = buffer[bufferPosition++];
        if (a == '<') a = '[';
        if (a == '>') a = ']';
        outBuf[copied++] = a;
        if (bufferPosition >= PROXY_BACKEND_BUFFER_SIZE) {
            bufferPosition = 0;
        }
        remaining--;
        bufferLength--;
    }
    if (copied < outBufSize) {
        outBuf[copied] = 0;
    }
    updateNonemptyFlag();
    return copied;
}

void ProxyLog_SetAttached(bool attached) {
    ProxyLog_IsAttached = attached;
}

static void addChar(char c) {
    if (CHAR_IS_VALID(c)) {
        if (bufferLength < PROXY_BACKEND_BUFFER_SIZE) {
            buffer[POS(bufferLength)] = c;
            bufferLength++;
        } else {
            buffer[bufferPosition++] = c;
            bufferPosition %= PROXY_BACKEND_BUFFER_SIZE;
        }

    }
}

void printToOurBuffer(uint8_t *data, size_t length) {
    for (uint16_t i = 0; i < length; i++) {
        addChar(data[i]);
    }
    updateNonemptyFlag();
}

static void processLog(const struct log_backend *const backend, union log_msg_generic *msg);

void panic(const struct log_backend *const backend) {
    StateWormhole_Open();
    StateWormhole.persistStatusBuffer = true;

    if (!isInPanicMode) {
        isInPanicMode = true;

        MacroStatusBuffer_Validate();
        printk("===== PANIC =====\n");
        Trace_Print("crash/panic");
    }

};

static int outputFunc(uint8_t *data, size_t length, void *ctx)
{
    if (isInPanicMode) {
        Macros_SanitizedPut(data, data + length);
    }
    if (ProxyLog_IsAttached) {
        printToOurBuffer(data, length);
    }
    return length;
}

// Allocate memory for actual log processing
#define LOG_BUFFER_SIZE 256
    static char logBuf[LOG_BUFFER_SIZE];
    static struct log_output_control_block logControlBlock;
    static struct log_output logOutput = {
        .func = outputFunc,
        .buf = logBuf,
        .size = LOG_BUFFER_SIZE,
        .control_block = &logControlBlock,
    };

static void processLog(const struct log_backend *const backend, union log_msg_generic *msg) {
    if (isInPanicMode || ProxyLog_IsAttached) {
        uint8_t flags = LOG_OUTPUT_FLAG_CRLF_LFONLY;
        log_output_msg_process(&logOutput, &msg->log, flags);
    }
}

void init(const struct log_backend *const backend) {};
int format_set(const struct log_backend *const backend, uint32_t log_type) { return 0; };
void notify(const struct log_backend *const backend, enum log_backend_evt event, union log_backend_evt_arg *arg) {};

static struct log_backend_api proxyApi = (struct log_backend_api) {
    .process = processLog,
    .dropped = NULL,
    .panic = panic,
    .init = init,
    .is_ready = NULL,
    .format_set = format_set,
    .notify = notify,
};

LOG_BACKEND_DEFINE(logProxy, proxyApi, false);

void InitProxyLogBackend(void) {
    log_init();

    const struct log_backend *backend = log_backend_get_by_name("logProxy");

    log_output_ctx_set(&logOutput, backend->cb->ctx);

    log_backend_activate(backend, backend->cb->ctx);
}

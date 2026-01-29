#include "proxy_log_backend.h"
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_msg.h>
#include <zephyr/logging/log_instance.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "attributes.h"
#include "logger.h"
#include "macros/status_buffer.h"
#include "trace.h"
#include "wormhole.h"
#include "zephyr/logging/log_core.h"
#include "config_manager.h"
#include "usb_log_buffer.h"

typedef struct {
    bool outputToStatusBuffer;
    bool outputToUsbBuffer;
    bool outputToOled;
} proxy_log_context_t;

#define PROXY_BACKEND_BUFFER_SIZE 2048

bool ProxyLog_IsInPanicMode = false;
bool ProxyLog_IsAttached = false;

void ProxyLog_SetAttached(bool attached) {
    ProxyLog_IsAttached = attached;
}

static void processLog(const struct log_backend *const backend, union log_msg_generic *msg);

void panic(const struct log_backend *const backend) {
    StateWormhole_Open();
    StateWormhole.persistStatusBuffer = true;

    if (!ProxyLog_IsInPanicMode) {
        ProxyLog_IsInPanicMode = true;

        MacroStatusBuffer_Validate();
        printk("===== PANIC =====\n");
        Trace_Print(LogTarget_ErrorBuffer, "crash/panic");
    }

};

static int outputFunc(uint8_t *data, size_t length, void *ctx)
{
    proxy_log_context_t* outputs = (proxy_log_context_t*)ctx;

    if (outputs->outputToStatusBuffer) {
        Macros_SanitizedPut(data, data + length);
    }
    if (outputs->outputToUsbBuffer) {
        UsbLogBuffer_Print(data, length);
    }
    if (outputs->outputToOled) {
        LogO("%.*s", length, data);
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

// Helper to get the log source name from a log message
ATTR_UNUSED static const char *getLogSourceName(struct log_msg *msg) {
    const void *source = log_msg_get_source(msg);
    if (source == NULL) {
        return "unknown";
    }
    // The source pointer is a log_source_const_data struct which has a name field
    const struct log_source_const_data *src_data = (const struct log_source_const_data *)source;
    return src_data->name ? src_data->name : "unnamed";
}

static void processLog(const struct log_backend *const backend, union log_msg_generic *msg) {
    proxy_log_context_t outputs = (proxy_log_context_t){
        .outputToStatusBuffer = false,
            .outputToUsbBuffer = false,
            .outputToOled = false,
    };

    if (ProxyLog_IsAttached) {
        outputs.outputToUsbBuffer = true;
        outputs.outputToOled = true;
    }

    if (ProxyLog_IsInPanicMode) {
        outputs.outputToStatusBuffer = true;
    }

    if (WormCfg->devMode) {
        // PoC: Extract log source and level information
        uint8_t level = log_msg_get_level(&msg->log);
        const char *sourceName = getLogSourceName(&msg->log);
        if ( level >= LOG_LEVEL_WRN && strcmp(sourceName, "c2usb") == 0) {
            outputs.outputToStatusBuffer = true;
        }
    }

    if (outputs.outputToStatusBuffer || outputs.outputToUsbBuffer || outputs.outputToOled) {
        log_output_ctx_set(&logOutput, &outputs);

        uint8_t flags = LOG_OUTPUT_FLAG_CRLF_LFONLY;
        log_output_msg_process(&logOutput, &msg->log, flags);
    }
}

void init(const struct log_backend *const backend) {};
int format_set(const struct log_backend *const backend, uint32_t log_type) { return 0; };
void notify(const struct log_backend *const backend, enum log_backend_evt event, union log_backend_evt_arg *arg) {};
static int is_ready(const struct log_backend *const backend) { return 0; }

static struct log_backend_api proxyApi = (struct log_backend_api) {
    .process = processLog,
    .dropped = NULL,
    .panic = panic,
    .init = init,
    .is_ready = is_ready,
    .format_set = format_set,
    .notify = notify,
};

#define PROXY_LOG_BACKEND_AUTOSTART true

LOG_BACKEND_DEFINE(logProxy, proxyApi, PROXY_LOG_BACKEND_AUTOSTART);

void InitProxyLogBackend(void) {
#if !PROXY_LOG_BACKEND_AUTOSTART
    log_init();

    const struct log_backend *backend = log_backend_get_by_name("logProxy");

    log_output_ctx_set(&logOutput, backend->cb->ctx);

    log_backend_activate(backend, backend->cb->ctx);
#endif
}

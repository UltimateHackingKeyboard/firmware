#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_msg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "logger.h"
#include "macros/status_buffer.h"
#include "shell/sinks.h"
#include "config_manager.h"
#include "usb_log_buffer.h"

// --- Log output callback ---

static int outputFunc(uint8_t *data, size_t length, void *ctx)
{
    shell_sinks_t *sinks = (shell_sinks_t *)ctx;

    if (sinks->toStatusBuffer) {
        Macros_SanitizedPut(data, data + length);
    }
    if (sinks->toUsbBuffer) {
        UsbLogBuffer_Print(data, length);
    }
    if (sinks->toOled) {
        LogO("%.*s", (int)length, data);
    }
    return length;
}

// --- Log output buffer ---

#define LOG_BUFFER_SIZE 256
static char logBuf[LOG_BUFFER_SIZE];
static struct log_output_control_block logControlBlock;
static struct log_output logOutput = {
    .func = outputFunc,
    .buf = logBuf,
    .size = LOG_BUFFER_SIZE,
    .control_block = &logControlBlock,
};

// --- Helpers ---

static const char *getLogSourceName(struct log_msg *msg)
{
    const void *source = log_msg_get_source(msg);
    if (source == NULL) {
        return "unknown";
    }
    const struct log_source_const_data *src_data = (const struct log_source_const_data *)source;
    return src_data->name ? src_data->name : "unnamed";
}

// --- Log backend API ---

static void proxyProcess(const struct log_backend *const backend, union log_msg_generic *msg)
{
    shell_sinks_t sinks = ShellConfig_GetLogSinks();

    if (WormCfg->devMode) {
        uint8_t level = log_msg_get_level(&msg->log);
        const char *sourceName = getLogSourceName(&msg->log);
        if (level >= LOG_LEVEL_WRN && strcmp(sourceName, "c2usb") == 0) {
            sinks.toStatusBuffer = true;
        }
    }

    if (sinks.toStatusBuffer || sinks.toUsbBuffer || sinks.toOled) {
        log_output_ctx_set(&logOutput, &sinks);
        uint8_t flags = LOG_OUTPUT_FLAG_CRLF_LFONLY;
        log_output_msg_process(&logOutput, &msg->log, flags);
    }
}

static void proxyPanic(const struct log_backend *const backend)
{
    ShellConfig_ActivatePanicMode();
}

static void proxyInit(const struct log_backend *const backend)
{
}

static int proxyIsReady(const struct log_backend *const backend)
{
    return 0;
}

static int proxyFormatSet(const struct log_backend *const backend, uint32_t log_type)
{
    return 0;
}

static void proxyNotify(const struct log_backend *const backend, enum log_backend_evt event, union log_backend_evt_arg *arg)
{
}

static const struct log_backend_api proxyApi = {
    .process = proxyProcess,
    .dropped = NULL,
    .panic = proxyPanic,
    .init = proxyInit,
    .is_ready = proxyIsReady,
    .format_set = proxyFormatSet,
    .notify = proxyNotify,
};

LOG_BACKEND_DEFINE(UhkLog, proxyApi, true);

#include "shell_log_backend.h"
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_msg.h>
#include <zephyr/drivers/uart.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "logger.h"
#include "macros/status_buffer.h"
#include "sinks.h"
#include "config_manager.h"
#include "usb_log_buffer.h"

// --- Sink log output ---

static int sinkOutputFunc(uint8_t *data, size_t length, void *ctx)
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

#define SINK_LOG_BUF_SIZE 256
static char sinkLogBuf[SINK_LOG_BUF_SIZE];
static struct log_output_control_block sinkLogControlBlock;
static struct log_output sinkLogOutput = {
    .func = sinkOutputFunc,
    .buf = sinkLogBuf,
    .size = SINK_LOG_BUF_SIZE,
    .control_block = &sinkLogControlBlock,
};

// --- Shell log output (writes directly to UART, bypassing shell internals) ---

static const struct device *uartDev;
static log_format_func_t logFormatFunc = log_output_msg_process;

/*
LOG_OUTPUT_FLAG_COLORS
LOG_OUTPUT_FLAG_TIMESTAMP
LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP
LOG_OUTPUT_FLAG_LEVEL
LOG_OUTPUT_FLAG_CRLF_NONE
LOG_OUTPUT_FLAG_CRLF_LFONLY
LOG_OUTPUT_FLAG_FORMAT_SYSLOG
LOG_OUTPUT_FLAG_THREAD
LOG_OUTPUT_FLAG_SKIP_SOURCE
*/
static uint32_t shellLogFlags = LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_COLORS;

static int uartOutputFunc(uint8_t *data, size_t length, void *ctx)
{
    const struct device *dev = (const struct device *)ctx;
    for (size_t i = 0; i < length; i++) {
        uart_poll_out(dev, data[i]);
    }
    return length;
}

#define SHELL_LOG_BUF_SIZE 256
static char shellLogBuf[SHELL_LOG_BUF_SIZE];
static struct log_output_control_block shellLogControlBlock;
static struct log_output shellLogOutput = {
    .func = uartOutputFunc,
    .buf = shellLogBuf,
    .size = SHELL_LOG_BUF_SIZE,
    .control_block = &shellLogControlBlock,
};

// --- Public API ---

void ShellLogBackend_SetUart(const struct device *dev)
{
    uartDev = dev;
    log_output_ctx_set(&shellLogOutput, (void *)dev);
}

void ShellLogBackend_ClearUart(void)
{
    uartDev = NULL;
}

void ShellLogBackend_SetLogFlags(uint32_t flags)
{
    shellLogFlags = flags;
}

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

static volatile bool shellOutputInProgress;

static void process(const struct log_backend *const backend, union log_msg_generic *msg)
{
    // Output to UART (reentrancy guard: UART output could theoretically trigger logging)
    if (uartDev != NULL && !shellOutputInProgress) {
        shellOutputInProgress = true;
        logFormatFunc(&shellLogOutput, &msg->log, shellLogFlags);
        shellOutputInProgress = false;
    }

    // Output to sinks (USB buffer, OLED, status buffer)
    shell_sinks_t sinks = ShellConfig_GetLogSinks();

    if (WormCfg->devMode) {
        uint8_t level = log_msg_get_level(&msg->log);
        const char *sourceName = getLogSourceName(&msg->log);
        if (level >= LOG_LEVEL_WRN && strcmp(sourceName, "c2usb") == 0) {
            sinks.toStatusBuffer = true;
        }
    }

    if (sinks.toStatusBuffer || sinks.toUsbBuffer || sinks.toOled) {
        log_output_ctx_set(&sinkLogOutput, &sinks);
        uint8_t flags = LOG_OUTPUT_FLAG_CRLF_LFONLY;
        log_output_msg_process(&sinkLogOutput, &msg->log, flags);
    }
}

static void panic(const struct log_backend *const backend)
{
    ShellConfig_ActivatePanicMode();
}

static void init(const struct log_backend *const backend)
{
}

static int isReady(const struct log_backend *const backend)
{
    return 0;
}

static int formatSet(const struct log_backend *const backend, uint32_t log_type)
{
    log_format_func_t func = log_format_func_t_get(log_type);
    if (func == NULL) {
        return -EINVAL;
    }
    logFormatFunc = func;
    return 0;
}

static void notify(const struct log_backend *const backend, enum log_backend_evt event, union log_backend_evt_arg *arg)
{
}

static const struct log_backend_api shellLogBackendApi = {
    .process = process,
    .dropped = NULL,
    .panic = panic,
    .init = init,
    .is_ready = isReady,
    .format_set = formatSet,
    .notify = notify,
};

LOG_BACKEND_DEFINE(UhkLog, shellLogBackendApi, true);

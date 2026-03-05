#include "shell_uhk.h"
#include "shell_transport_uhk.h"
#include "shell_log_backend.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/mpsc_pbuf.h>

// --- Configuration ---

#define SHELL_LOG_QUEUE_SIZE 512
#define SHELL_LOG_QUEUE_TIMEOUT 1000

// --- Shell instance ---

SHELL_DEFINE(UhkShell, CONFIG_SHELL_PROMPT_RTT, &ShellUartTransport,
             SHELL_LOG_QUEUE_SIZE,
             SHELL_LOG_QUEUE_TIMEOUT,
             SHELL_FLAG_OLF_CRLF);

// --- Public API ---

const struct shell *ShellUhk_GetShellPtr(void)
{
    return &UhkShell;
}

int ShellUhk_Init(void)
{
    const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
    bool log_backend = false;
    uint32_t level = CONFIG_LOG_MAX_LEVEL;
    static const struct shell_backend_config_flags cfg_flags =
        SHELL_DEFAULT_BACKEND_CONFIG_FLAGS;

    if (!device_is_ready(dev)) {
        return -ENODEV;
    }

    shell_init(&UhkShell, dev, cfg_flags, log_backend, level);

    // With log_backend=false, the shell's built-in log backend is never enabled, but
    // the shell thread still calls z_shell_log_backend_process() unconditionally.
    // Initialize the MPSC buffer and set the context so it finds an empty buffer
    // instead of dereferencing NULL/garbage.
    mpsc_pbuf_init(UhkShell.log_backend->mpsc_buffer,
                   UhkShell.log_backend->mpsc_buffer_config);
    UhkShell.log_backend->backend->cb->ctx = (void *)&UhkShell;

    ShellLogBackend_SetUart(dev);

    return 0;
}

SYS_INIT(ShellUhk_Init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);

void Shell_WaitUntilInitialized(void)
{
    const struct shell *sh = ShellUhk_GetShellPtr();
    if (sh) {
        // if we set levels before shell is ready, the shell will mercilessly overwrite them
        while (!shell_ready(sh)) {
            k_msleep(10);
        }
    }
}

void Shell_Execute(const char *cmd, const char *source)
{
    const struct shell *sh = ShellUhk_GetShellPtr();

    if (sh == NULL) {
        printk("Shell backend not available, cannot execute command: '%s'\n", cmd);
        return;
    }

    /* Ensure terminal width is set to prevent division by zero errors */
    if (sh->ctx != NULL && sh->ctx->vt100_ctx.cons.terminal_wid == 0) {
        struct shell *sh_mutable = (struct shell *)sh;
        sh_mutable->ctx->vt100_ctx.cons.terminal_wid = 80;
    }

    printk("Executing following command from %s in %s: '%s'\n", source ? source : "unknown", sh->name, cmd);
    shell_execute_cmd(sh, cmd);
}

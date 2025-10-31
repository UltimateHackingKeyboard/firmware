#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_backend.h>
#include <zephyr/logging/log.h>
#include <string.h>

#ifdef CONFIG_SHELL_BACKEND_SERIAL
#include <zephyr/shell/shell_uart.h>
#endif

/* Get an existing, properly initialized shell backend for command execution */
static const struct shell *get_shell_instance(void)
{
    const struct shell *sh = NULL;

    /* If no UART backend, try to get any backend */
    size_t backend_count = shell_backend_count_get();
    if (backend_count > 0) {
        for (size_t i = 0; i < backend_count; i++) {
            sh = shell_backend_get(i);
            if (sh != NULL) {
                printk("executing over %s\n", sh->name ? sh->name : "<unnamed>");
                return sh;
            }
        }
    }

    /* Fallback: return NULL, caller should handle */
    return NULL;
}

void ShellBackend_Exec(const char *cmd, const char* source)
{
    const struct shell *sh = get_shell_instance();

    if (sh == NULL) {
        printk("Shell backend not available, cannot execute command: '%s'\n", cmd);
        return;
    }

    /* Ensure terminal width is set to prevent division by zero errors */
    /* Some backends (bt_nus, rtt) don't initialize terminal width automatically */
    if (sh->ctx != NULL && sh->ctx->vt100_ctx.cons.terminal_wid == 0) {
        /* Cast away const to set terminal width */
        struct shell *sh_mutable = (struct shell *)sh;
        sh_mutable->ctx->vt100_ctx.cons.terminal_wid = 80; /* Default to 80 columns */
    }

    printk("Executing following command from %s: '%s'\n", source ? source : "unknown", cmd);
    shell_execute_cmd(sh, cmd);
}

void ShellBackend_ListBackends(void)
{
    size_t backend_count = shell_backend_count_get();

    if (backend_count == 0) {
        printk("No shell backends available\n");
        return;
    }

    printk("Available shell backends (%zu):\n", backend_count);

    for (size_t i = 0; i < backend_count; i++) {
        const struct shell *sh = shell_backend_get(i);
        if (sh != NULL) {
            printk("  [%zu] %s\n", i, sh->name ? sh->name : "<unnamed>");
        }
    }
}

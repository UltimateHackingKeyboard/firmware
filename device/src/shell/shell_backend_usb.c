#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_backend.h>
#include <zephyr/logging/log.h>
#include <string.h>

static const struct shell *get_shell_backend_by_name(const char *name)
{
    size_t backend_count = shell_backend_count_get();
    for (size_t i = 0; i < backend_count; i++) {
        const struct shell *sh = shell_backend_get(i);
        if (sh != NULL && sh->name != NULL && strcmp(sh->name, name) == 0) {
            return sh;
        }
    }
    return NULL;
}

void ShellBackend_Exec(const char *cmd, const char* source)
{
    const struct shell *sh = get_shell_backend_by_name("UhkShell");

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

    printk("Executing following command from %s in %s: '%s'\n", source ? source : "unknown", sh->name, cmd);
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

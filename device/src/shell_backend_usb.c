#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>

static void fwrite(const void *user_ctx, const char *data, size_t len)
{
    printk("%.*s", (int)len, (const char *)data);
}

/* Dummy fprintf backend for the shell */
static const struct shell_fprintf log_fprintf = {
    .fwrite = fwrite,
    .user_ctx = NULL,
};

/* Global headless shell instance */
static const struct shell headless_shell = {
    .fprintf_ctx = (struct shell_fprintf *)&log_fprintf,
    .ctx = NULL,
    .iface = NULL,
    .default_prompt = "headless:~$ ",
};

void ShellBackend_Exec(const char *cmd, const char* source)
{
    printk("Executing following command from %s: '%s'\n", source, cmd);
    shell_execute_cmd(&headless_shell, cmd);
}

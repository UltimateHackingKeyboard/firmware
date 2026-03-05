#ifndef __SHELL_UHK_H__
#define __SHELL_UHK_H__

// Includes:

    #include <zephyr/shell/shell.h>

// Functions:

    const struct shell *ShellUhk_GetShellPtr(void);
    int ShellUhk_Init(void);
    void Shell_WaitUntilInitialized(void);
    void Shell_Execute(const char *cmd, const char *source);

#endif

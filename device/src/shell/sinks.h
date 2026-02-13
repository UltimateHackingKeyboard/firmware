#ifndef __SINKS_H__
#define __SINKS_H__

// Includes:

    #include <stdbool.h>

// Typedefs:

    typedef struct {
        bool toUsbBuffer;
        bool toOled;
        bool toStatusBuffer;
    } shell_sinks_t;

// Variables:

    extern bool ShellConfig_IsInPanicMode;

// Functions:

    void ShellConfig_ActivatePanicMode(void);
    shell_sinks_t ShellConfig_GetShellSinks(void);
    shell_sinks_t ShellConfig_GetLogSinks(void);

#endif

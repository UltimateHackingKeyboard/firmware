#ifndef __SHELL_BACKEND_USB_H__
#define __SHELL_BACKEND_USB_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>

// Typedefs:

// Variables:

// Functions:

    void ShellBackend_Init(void);
    void ShellBackend_Exec(const char *cmd, const char* source);
    void ShellBackend_ListBackends(void);

#endif /* __SHELL_BACKEND_USB_H__ */


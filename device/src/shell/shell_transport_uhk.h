#ifndef __SHELL_TRANSPORT_UHK_H__
#define __SHELL_TRANSPORT_UHK_H__

// Includes:

    #include <zephyr/shell/shell.h>

// Variables:

    extern struct shell_transport ShellUartTransport;

// Functions:

    void ShellUartTransport_Uninit(void);
    void ShellUartTransport_Reinit(void);

#endif

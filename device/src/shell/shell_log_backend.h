#ifndef __SHELL_LOG_BACKEND_UHK_H__
#define __SHELL_LOG_BACKEND_UHK_H__

// Includes:

    #include <zephyr/device.h>
    #include <stdint.h>

// Functions:

    void ShellLogBackend_SetUart(const struct device *dev);
    void ShellLogBackend_ClearUart(void);
    void ShellLogBackend_SetLogFlags(uint32_t flags);

#endif

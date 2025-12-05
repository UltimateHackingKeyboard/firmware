#ifndef __PIN_WIRING_H__
#define __PIN_WIRING_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>
    #include "device.h"
    #include "connections.h"
    #include "messenger.h"
    #include <zephyr/drivers/pinctrl.h>
    #include "settings.h"

// Macros:


// Typedefs:

    typedef struct {
        const char* name;
        const struct device* device;
        struct pinctrl_dev_config* pinctrl;
    } ATTR_PACKED pin_wiring_dev_t;

    typedef struct {
        const struct pinctrl_state* pins_uart0;
        const struct pinctrl_state* pins_uart1;
        const struct pinctrl_state* pins_i2c;
        const pin_wiring_dev_t* device_uart_shell;
        const pin_wiring_dev_t* device_uart_bridge;
        const pin_wiring_dev_t* device_uart_modules;
        const pin_wiring_dev_t* device_i2c_modules;
    } ATTR_PACKED pin_wiring_config_t;

// Variables:

    extern const pin_wiring_config_t* PinWiringConfig;
    extern uart_debug_mode_t PinWiring_ActualUartDebugMode;

// Functions:

    void PinWiring_UninitShell();
    void PinWiring_SelectRouting();
    void PinWiring_Suspend();
    void PinWiring_ConfigureRouting();
    void PinWiring_Resume();

#endif

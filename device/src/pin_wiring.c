#include "attributes.h"
#include "shell.h"
#include "bt_conn.h"
#include "device.h"
#include "keyboard/charger.h"
#include "keyboard/leds.h"
#include "keyboard/oled/oled.h"
#include "logger.h"
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>
#include "bt_conn.h"
#include "keyboard/charger.h"
#include "ledmap.h"
#include "event_scheduler.h"
#include "host_connection.h"
#include "thread_stats.h"
#include "trace.h"
#include "mouse_keys.h"
#include "config_manager.h"
#include <zephyr/shell/shell_backend.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/shell/shell.h>
#include "connections.h"
#include "logger_priority.h"
#include "pin_wiring.h"
#include "device.h"
#include "logger.h"
#include "stubs.h"
#include "macros/status_buffer.h"
#include "pin_wiring.h"
#include "device.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include "debug.h"
#include "settings.h"
#include "keyboard/oled/widgets/widgets.h"
#include "stubs.h"

uart_debug_mode_t PinWiring_ActualUartDebugMode = 0;

#if !DEVICE_IS_UHK_DONGLE
#if DEVICE_IS_UHK80_RIGHT

#define DEFINE_PIN_STATE(NAME) \
    PINCTRL_DT_STATE_PINS_DEFINE(DT_PATH(zephyr_user), NAME##_default); \
    PINCTRL_DT_STATE_PINS_DEFINE(DT_PATH(zephyr_user), NAME##_sleep);

#define EXPAND_PIN_STATE(NAME) \
    ATTR_UNUSED static const struct pinctrl_state NAME##_pins[] = { \
        PINCTRL_DT_STATE_INIT(NAME##_default, PINCTRL_STATE_DEFAULT), \
        PINCTRL_DT_STATE_INIT(NAME##_sleep, PINCTRL_STATE_SLEEP), \
    };

#define EXPAND_DEVICE(NAME) \
    PINCTRL_DT_DEV_CONFIG_DECLARE(DT_NODELABEL(NAME)); \
    ATTR_UNUSED const pin_wiring_dev_t NAME = { \
        .name = #NAME, \
        .device = DEVICE_DT_GET(DT_NODELABEL(NAME)), \
        .pinctrl = PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(NAME)), \
    };

#else

#define DEFINE_PIN_STATE(NAME)
#define EXPAND_PIN_STATE(NAME) const struct pinctrl_state* const NAME##_pins = NULL;
#define EXPAND_DEVICE(NAME) \
    ATTR_UNUSED const pin_wiring_dev_t NAME = { \
        .name = #NAME, \
        .device = DEVICE_DT_GET(DT_NODELABEL(NAME)), \
        .pinctrl = NULL, \
    };

#endif

DEFINE_PIN_STATE(uart_swd);
DEFINE_PIN_STATE(uart_bridge);
DEFINE_PIN_STATE(uart_modules);
DEFINE_PIN_STATE(i2c_modules);

EXPAND_PIN_STATE(uart_swd);
EXPAND_PIN_STATE(uart_bridge);
EXPAND_PIN_STATE(i2c_modules);
EXPAND_PIN_STATE(uart_modules);

EXPAND_DEVICE(uart0);
EXPAND_DEVICE(uart1);
EXPAND_DEVICE(i2c0);

const pin_wiring_config_t Uhk80_I2cModules = {
    .pins_uart0 = uart_swd_pins,
    .pins_uart1 = uart_bridge_pins,
    .pins_i2c = i2c_modules_pins,
    .device_i2c_modules = &i2c0,
    .device_uart_bridge = &uart1,
    .device_uart_shell = &uart0,
    .device_uart_modules = NULL,
};

const pin_wiring_config_t Uhk80_NoDebug = {
    .pins_uart0 = uart_bridge_pins,
    .pins_uart1 = uart_modules_pins,
    .pins_i2c = NULL,
    .device_i2c_modules = NULL,
    .device_uart_bridge = &uart0,
    .device_uart_shell = NULL,
    .device_uart_modules = &uart1,
};

const pin_wiring_config_t Uhk80_NoBridge = {
    .pins_uart0 = uart_swd_pins,
    .pins_uart1 = uart_modules_pins,
    .pins_i2c = NULL,
    .device_i2c_modules = NULL,
    .device_uart_bridge = NULL,
    .device_uart_shell = &uart0,
    .device_uart_modules = &uart1,
};

const pin_wiring_config_t Uhk80_NoModules = {
    .pins_uart0 = uart_swd_pins,
    .pins_uart1 = uart_bridge_pins,
    .pins_i2c = NULL,
    .device_i2c_modules = NULL,
    .device_uart_bridge = &uart1,
    .device_uart_shell = &uart0,
    .device_uart_modules = NULL,
};


const pin_wiring_config_t *PinWiringConfig = &Uhk80_I2cModules;

ATTR_UNUSED void logDeviceState(const struct device* dev, const char* label) {
    enum pm_device_state state;

    int ret = pm_device_state_get(dev, &state);
    if (ret != 0) {
        LogS("Failed to get device state: %d at %s\n", ret, label);
        return;
    }
    LogS("Device %s state at %s: %s\n", dev->name, label, pm_device_state_str(state));
}

void configurePins(const pin_wiring_dev_t* dev, const struct pinctrl_state* states) {
#if DEVICE_IS_UHK80_RIGHT
    if (dev == NULL || states == NULL) {
        return;
    }

    uint8_t supportedStateCount = dev->pinctrl->state_cnt;
    int ret;

    ret = pinctrl_update_states(dev->pinctrl, states, supportedStateCount);
    if (ret != 0) {
        LogS("Failed to update pin states: %d\n", ret);
    }
#else
    Macros_ReportError("configurePins called on unsupported device", NULL, NULL);

#endif
}


void uninitUart(const pin_wiring_dev_t* dev) {
    if (dev == NULL) {
        return;
    }
    int err = 0;

    err = uart_rx_disable(dev->device);
    if (err != 0) {
        LogS("Failed to disable UART RX: %d\n", err);
    }

    k_sleep(K_MSEC(10));
}


void suspendDevice(const pin_wiring_dev_t* dev) {
    if (dev == NULL) {
        return;
    }

    int ret;

    if (dev->device == NULL) {
        LogS("Device %s is NULL, cannot suspend\n", dev->name);
        k_sleep(K_MSEC(2000));
        return;
    }

    ret = pm_device_action_run(dev->device, PM_DEVICE_ACTION_SUSPEND);
    if (ret != 0) {
        LogS("Failed to suspend device: %d\n", ret);
    }
}

void resumeDevice(const pin_wiring_dev_t* dev) {
    if (dev == NULL) {
        return;
    }

    int ret = pm_device_action_run(dev->device, PM_DEVICE_ACTION_RESUME);
    if (ret != 0) {
        LogS("Failed to resume device: %d\n", ret);
    }
}

static void selectMode() {
    PinWiring_ActualUartDebugMode = Settings_UartDebugMode;

    switch(PinWiring_ActualUartDebugMode) {
        case UartDebugMode_NoDebug:
            PinWiringConfig = &Uhk80_NoDebug;
            break;
        case UartDebugMode_DebugOverBridge:
            PinWiringConfig = &Uhk80_NoBridge;
            break;
        case UartDebugMode_DebugOverModules:
            PinWiringConfig = &Uhk80_NoModules;
            break;
        case UartDebugMode_I2CMode:
            PinWiringConfig = &Uhk80_I2cModules;
            break;
    }
}

void PinWiring_SelectRouting(void) {
    selectMode();
}

void PinWiring_UninitShell(void) {
    UninitShell();
    uninitUart(&uart0);
}

void PinWiring_Suspend(void) {
    suspendDevice(&uart0);
    suspendDevice(&uart1);
    suspendDevice(&i2c0);
}


void PinWiring_ConfigureRouting(void) {
    configurePins(&uart0, PinWiringConfig->pins_uart0);
    configurePins(&uart1, PinWiringConfig->pins_uart1);
    configurePins(&i2c0, PinWiringConfig->pins_i2c);

    WIDGET_REFRESH(&StatusWidget);
}

void PinWiring_Resume(void) {
    resumeDevice(PinWiringConfig->device_uart_shell);
    resumeDevice(PinWiringConfig->device_uart_bridge);
    resumeDevice(PinWiringConfig->device_uart_modules);
    resumeDevice(PinWiringConfig->device_i2c_modules);
}
#endif

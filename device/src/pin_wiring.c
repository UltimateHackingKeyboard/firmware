#include "pin_wiring.h"
#include "device.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include "debug.h"

#define STATES_PER_DEVICE 2

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
    .device_i2c_module = &i2c0,
    .device_uart_bridge = &uart1,
    .device_uart_swd = &uart0,
    .device_uart_module = NULL,
};

const pin_wiring_config_t Uhk80_Testing = {
    .pins_uart0 = uart_swd_pins,
    .pins_uart1 = NULL,
    .pins_i2c = i2c_modules_pins,
    .device_i2c_module = &i2c0,
    .device_uart_bridge = &uart0,
    .device_uart_swd = NULL,
    .device_uart_module = NULL,
};

const pin_wiring_config_t Uhk80_NoSwd = {
    .pins_uart0 = uart_bridge_pins,
    .pins_uart1 = uart_modules_pins,
    .pins_i2c = NULL,
    .device_i2c_module = NULL,
    .device_uart_bridge = &uart0,
    .device_uart_swd = NULL,
    .device_uart_module = &uart1,
};

const pin_wiring_config_t *PinWiringConfig;

void logDeviceState(const struct device* dev, const char* label) {
    enum pm_device_state state;

    int ret = pm_device_state_get(dev, &state);
    if (ret != 0) {
        LogS("Failed to get device state: %d at %s\n", ret, label);
        return;
    }
    LogS("Device %s state at %s: %s\n", dev->name, label, pm_device_state_str(state));
}

void configurePins(const pin_wiring_dev_t* dev, const struct pinctrl_state* states) {
    if (dev == NULL || states == NULL) {
        return;
    }

    uint8_t supportedStateCount = dev->pinctrl->state_cnt;
    int ret;

    ret = pinctrl_update_states(dev->pinctrl, states, supportedStateCount);
    if (ret != 0) {
        LogS("Failed to update pin states: %d\n", ret);
    }
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
    LogS("Suspended device %s\n", dev->name);
}

void resumeDevice(const pin_wiring_dev_t* dev) {
    if (dev == NULL) {
        return;
    }

    int ret = pm_device_action_run(dev->device, PM_DEVICE_ACTION_RESUME);
    if (ret != 0) {
        LogS("Failed to resume device: %d\n", ret);
    }

    LogS("Resumed device %s\n", dev->name);
}

void InitPinWiring(void) {
    LogS("B0");
    k_sleep(K_MSEC(2000));

    PinWiringConfig = &Uhk80_I2cModules;
    // PinWiringConfig = &Uhk80_Testing;

    LogS("B1");
    k_sleep(K_MSEC(2000));

    suspendDevice(&uart0);

    logDeviceState(uart0.device, "B11");
    LogS("B11");
    k_sleep(K_MSEC(2000));
    // suspendDevice(&uart1);

    LogS("B12");
    k_sleep(K_MSEC(2000));

    // suspendDevice(&i2c0);

    LogS("B2");
    k_sleep(K_MSEC(2000));

    configurePins(&uart0, PinWiringConfig->pins_uart0);
    configurePins(&uart1, PinWiringConfig->pins_uart1);
    configurePins(&i2c0, PinWiringConfig->pins_i2c);

    LogS("B3");
    k_sleep(K_MSEC(2000));

    resumeDevice(PinWiringConfig->device_uart_swd);
    resumeDevice(PinWiringConfig->device_uart_bridge);
    resumeDevice(PinWiringConfig->device_uart_module);
    resumeDevice(PinWiringConfig->device_i2c_module);

    LogS("B4");
    k_sleep(K_MSEC(2000));
}


#include <zephyr/shell/shell.h>
extern "C"
{
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/usb/class/usb_hid.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <soc.h>
#include <assert.h>

#include "bluetooth.h"
#include "key_scanner.h"
#include "leds.h"
#include "oled.h"
#include "charger.h"
#include "spi.h"
#include "uart.h"
}

#include "usb/usb.hpp"
#include "usb/keyboard_app.hpp"
#include "usb/mouse_app.hpp"
#include "usb/controls_app.hpp"
#include "usb/gamepad_app.hpp"
#include <zephyr/drivers/adc.h>
#include "device.h"

// Shell functions

uint8_t keyLog = 1;
static int cmd_uhk_keylog(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "%i\n", keyLog);
    } else {
        keyLog = argv[1][0] == '1';
    }
    return 0;
}

uint8_t statLog = 1;
static int cmd_uhk_statlog(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "%i\n", statLog);
    } else {
        statLog = argv[1][0] == '1';
    }
    return 0;
}

uint8_t ledsAlwaysOn = 0;
static int cmd_uhk_leds(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "%i\n", ledsAlwaysOn);
    } else {
        ledsAlwaysOn = argv[1][0] == '1';
    }
    return 0;
}

uint8_t sdbState = 1;
static int cmd_uhk_sdb(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "%i\n", sdbState ? 1 : 0);
    } else {
        sdbState = argv[1][0] == '1';
        gpio_pin_set_dt(&ledsSdbDt, sdbState);
    }
    return 0;
}

// Charger behavior:
// - If CHARGER_EN=0 or USB is disconnected, then TS reads 6552[0-9] raw value and sometimes drops to 0.
// - If CHARGER_EN=1, USB is connected, and the battery is disconnected, then STAT alternates between 0 and 1 per second

uint8_t chargerState = 1;
static int cmd_uhk_charger(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "CHARGER_EN: %i | ", chargerState ? 1 : 0);
        shell_fprintf(shell, SHELL_NORMAL, "STAT: %i", gpio_pin_get_dt(&chargerStatDt) ? 1 : 0);

        int err;
        uint16_t buf;
        struct adc_sequence sequence = {
            .buffer = &buf,
            .buffer_size = sizeof(buf),
        };

        for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
            int32_t val_mv;

            printk(" | ");
            printk(i ? "VBAT" : "TS");

            (void)adc_sequence_init_dt(&adc_channels[i], &sequence);

            err = adc_read(adc_channels[i].dev, &sequence);
            if (err < 0) {
                printk("Could not read (%d)\n", err);
                continue;
            }

            // If using differential mode, the 16 bit value
            // in the ADC sample buffer should be a signed 2's
            // complement value.
            if (adc_channels[i].channel_cfg.differential) {
                val_mv = (int32_t)((int16_t)buf);
            } else {
                val_mv = (int32_t)buf;
            }
            printk(": %d", val_mv);
            err = adc_raw_to_millivolts_dt(&adc_channels[i], &val_mv);
            // conversion to mV may not be supported, skip if not
            if (err < 0) {
                printk(" (value in mV not available)");
            } else {
                printk(" = %d mV", val_mv);
            }
        }
        printk("\n");
    } else {
        chargerState = argv[1][0] == '1';
        gpio_pin_set_dt(&chargerEnDt, chargerState);
    }
    return 0;
}

#ifdef DEVICE_HAS_OLED
uint8_t oledState = 1;
static int cmd_uhk_oled(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "%i\n", oledState ? 1 : 0);
    } else {
        oledState = argv[1][0] == '1';
        gpio_pin_set_dt(&oledEn, oledState);
    }
    return 0;
}
#endif

#ifdef DEVICE_HAS_MERGE_SENSE
static const struct gpio_dt_spec mergeSenseDt = GPIO_DT_SPEC_GET(DT_ALIAS(merge_sense), gpios);

static int cmd_uhk_merge(const struct shell *shell, size_t argc, char *argv[])
{
    shell_fprintf(shell, SHELL_NORMAL, "%i\n", gpio_pin_get_dt(&mergeSenseDt) ? 1 : 0);
    return 0;
}
#endif

static int cmd_uhk_rollover(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "%c\n",
                (keyboard_app::handle().get_rollover() == keyboard_app::rollover::N_KEY) ? 'n' : '6');
    } else {
        keyboard_app::handle().set_rollover((argv[1][0] == '6') ?
                keyboard_app::rollover::SIX_KEY : keyboard_app::rollover::N_KEY);
    }
    return 0;
}

static int cmd_uhk_gamepad(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        // TODO
    } else {
        usb_init(argv[1][0] == '1');
    }
    return 0;
}

int main(void) {
    printk("----------\n" DEVICE_NAME " started\n");

    // Configure GPIOs

    InitUart();
    k_mutex_init(&SpiMutex);
    InitOled();
    InitLeds();
    InitCharger();

#ifdef DEVICE_HAS_MERGE_SENSE
    gpio_pin_configure_dt(&mergeSenseDt, GPIO_INPUT);
#endif

    // Create shell commands

    SHELL_STATIC_SUBCMD_SET_CREATE(
        uhk_cmds,
        SHELL_CMD_ARG(keylog, NULL,
            "get/set key logging",
            cmd_uhk_keylog, 1, 1),
        SHELL_CMD_ARG(statlog, NULL,
            "get/set stat logging",
            cmd_uhk_statlog, 1, 1),
        SHELL_CMD_ARG(leds, NULL,
            "get/set LEDs always on state",
            cmd_uhk_leds, 1, 1),
        SHELL_CMD_ARG(sdb, NULL,
            "get/set LED driver SDB pin",
            cmd_uhk_sdb, 1, 1),
        SHELL_CMD_ARG(charger, NULL,
            "get/set CHARGER_EN pin",
            cmd_uhk_charger, 1, 1),
#ifdef DEVICE_HAS_OLED
        SHELL_CMD_ARG(oled, NULL,
            "get/set OLED_EN pin",
            cmd_uhk_oled, 1, 1),
#endif
#ifdef DEVICE_HAS_MERGE_SENSE
        SHELL_CMD_ARG(merge, NULL,
            "get the merged state of UHK halves",
            cmd_uhk_merge, 1, 0),
#endif
        SHELL_CMD_ARG(rollover, NULL,
            "get/set keyboard rollover mode (n/6)",
            cmd_uhk_rollover, 1, 1),
        SHELL_CMD_ARG(gamepad, NULL,
            "switch gamepad on/off",
            cmd_uhk_gamepad, 1, 1),
        SHELL_SUBCMD_SET_END
    );

    SHELL_CMD_REGISTER(uhk, &uhk_cmds, "UHK commands", NULL);

    // Init I2C
#if CONFIG_DEVICE_ID == DEVICE_ID_UHK80_LEFT
    #define device_addr 0x18 // left module i2c address
#elif CONFIG_DEVICE_ID == DEVICE_ID_UHK80_RIGHT || CONFIG_DEVICE_ID == DEVICE_ID_UHK60V1_RIGHT || CONFIG_DEVICE_ID == DEVICE_ID_UHK60V2_RIGHT
    #define device_addr 0x28 // right module i2c address
#endif

    uint8_t tx_buf[] = {0x00,0x00};
    uint8_t rx_buf[10] = {0};

    int ret;
    static const struct device *i2c0_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
    k_msleep(50);
    if (!device_is_ready(i2c0_dev)) {
        printk("I2C bus %s is not ready!\n",i2c0_dev->name);
    }

    ret = i2c_write_read(i2c0_dev, device_addr, tx_buf, 2, rx_buf, 7);
    if (ret != 0) {
        printk("write-read fail\n");
    }
    printk("sync: %.7s\n", rx_buf);

    // Init ADC channels
    for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
        if (!device_is_ready(adc_channels[i].dev)) {
            printk("ADC controller device %s not ready\n", adc_channels[i].dev->name);
            return 0;
        }
        int err;
        err = adc_channel_setup_dt(&adc_channels[i]);
        if (err < 0) {
            printk("Could not setup channel #%d (%d)\n", i, err);
            return 0;
        }
    }


    // dk_buttons_init(button_changed);
    // dk_leds_init();

    usb_init(true);
    bluetooth_init();

//  int blink_status = 0;
    scancode_buffer prevKeys, keys;
    mouse_buffer prevMouseState, mouseState;
    controls_buffer prevControls, controls;
    gamepad_buffer prevGamepad, gamepad;

    InitKeyScanner();

    for (;;) {
        keys.set_code(scancode::A, KeyStates[0][0]);
        if (keys != prevKeys) {
            auto result = keyboard_app::handle().send(keys);
            if (result == hid::result::OK) {
                // buffer accepted for transmit
                prevKeys = keys;
            }
        }

        mouseState.set_button(mouse_button::RIGHT, KeyStates[0][1]);
        mouseState.x = -50;
        // mouseState.y = -50;
        // mouseState.wheel_y = -50;
        // mouseState.wheel_x = -50;
        if (mouseState != prevMouseState) {
            auto result = mouse_app::handle().send(mouseState);
            if (result == hid::result::OK) {
                // buffer accepted for transmit
                prevMouseState = mouseState;
            }
        }

        controls.set_code(consumer_code::VOLUME_INCREMENT, KeyStates[0][2]);
        if (controls != prevControls) {
            auto result = controls_app::handle().send(controls);
            if (result == hid::result::OK) {
                // buffer accepted for transmit
                prevControls = controls;
            }
        }

        gamepad.set_button(gamepad_button::X, KeyStates[0][3]);
        // gamepad.left.X = 50;
        // gamepad.right.Y = 50;
        // gamepad.right_trigger = 50;
        if (gamepad != prevGamepad) {
            auto result = gamepad_app::handle().send(gamepad);
            if (result == hid::result::OK) {
                // buffer accepted for transmit
                prevGamepad = gamepad;
            }
        }
//      bluetooth_set_adv_led(&blink_status);
//      k_sleep(K_MSEC(ADV_LED_BLINK_INTERVAL));
        // Battery level simulation
        bas_notify();

        k_msleep(1);
    }
}

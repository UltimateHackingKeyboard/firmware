#include "keyboard/power.h"
#include "zephyr/storage/flash_map.h"
#include "keyboard/key_scanner.h"
#include "keyboard/leds.h"
#include "keyboard/oled/oled.h"
#include "keyboard/charger.h"
#include "keyboard/spi.h"
#include "keyboard/uart.h"
#include "nus_client.h"
#include "nus_server.h"
#include "keyboard/i2c.h"
#include "peripherals/merge_sensor.h"
#include "shell.h"
#include "device.h"
#include "usb/usb.h"
#include "bt_conn.h"
#include "bt_advertise.h"
#include "settings.h"
#include "flash.h"
#include "usb_commands/usb_command_apply_config.h"
#include "macros/shortcut_parser.h"
#include "macros/keyid_parser.h"
#include "macros/core.h"
#include "legacy/timer.h"
#include "legacy/user_logic.h"
#include "state_sync.h"
#include "legacy/config_manager.h"
#include "keyboard/power.h"
#include "messenger.h"
#include "legacy/led_manager.h"
// #include <zephyr/drivers/gpio.h>
// #include "dongle_leds.h"

int main(void) {
    printk("----------\n" DEVICE_NAME " started\n");

    // const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0_green), gpios);
    // gpio_pin_configure_dt(&led0, GPIO_OUTPUT);
    // while (true) {
    //     gpio_pin_set_dt(&led0, true);
    //     k_sleep(K_MSEC(1000));
    //     gpio_pin_set_dt(&led0, false);
    //     set_dongle_led(&red_pwm_led, 100);
    //     k_sleep(K_MSEC(1000));
    //     set_dongle_led(&red_pwm_led, 0);
    //     set_dongle_led(&green_pwm_led, 100);
    //     k_sleep(K_MSEC(1000));
    //     set_dongle_led(&green_pwm_led, 0);
    //     set_dongle_led(&blue_pwm_led, 100);
    //     k_sleep(K_MSEC(1000));
    //     set_dongle_led(&blue_pwm_led, 0);
    // }

    if (DEVICE_IS_UHK80_RIGHT) {
        flash_area_open(FLASH_AREA_ID(hardware_config_partition), &hardwareConfigArea);
        flash_area_open(FLASH_AREA_ID(user_config_partition), &userConfigArea);
    }

    if (!DEVICE_IS_UHK_DONGLE) {
        InitUart();
        InitI2c();
        InitSpi();

        #ifdef DEVICE_HAS_OLED
        InitOled();
        #endif // DEVICE_HAS_OLED

        InitLeds();
        InitCharger();

    #ifdef DEVICE_HAS_MERGE_SENSOR
        MergeSensor_Init();
    #endif // DEVICE_HAS_MERGE_SENSOR

        InitKeyScanner();

    }
    USB_EnableHid();

    bt_init();
    InitSettings();

    if (DEVICE_IS_UHK80_LEFT) {
        int err = NusServer_Init();
        if (!err) {
            Advertise(ADVERTISE_NUS);
        }
    } else if (DEVICE_IS_UHK80_RIGHT) {
        HOGP_Enable();
        Advertise(ADVERTISE_NUS | ADVERTISE_HID);
    }

    if (DEVICE_IS_UHK80_RIGHT || DEVICE_IS_UHK_DONGLE) {
        NusClient_Init();
    }

    if (DEVICE_IS_UHK80_LEFT || DEVICE_IS_UHK80_RIGHT) {
        ConfigManager_ResetConfiguration(false);
    }

    if (DEVICE_IS_UHK80_RIGHT) {
        printk("Reading hardware config\n");
        flash_area_read(hardwareConfigArea, 0, HardwareConfigBuffer.buffer, HARDWARE_CONFIG_SIZE);
        printk("Reading user config\n");
        flash_area_read(userConfigArea, 0, StagingUserConfigBuffer.buffer, USER_CONFIG_SIZE);
        printk("Applying user config\n");
        bool factoryMode = false;
        if (factoryMode) {
            LedManager_FullUpdate();
        } else {
            UsbCommand_ApplyConfig();
        }
        printk("User config applied\n");
        ShortcutParser_initialize();
        KeyIdParser_initialize();
        Macros_Initialize();
    }

    Messenger_Init();

    if (DEVICE_IS_UHK80_LEFT || DEVICE_IS_UHK80_RIGHT) {
        StateSync_Init();
    }

#if DEVICE_IS_UHK80_RIGHT
    while (true)
    {
        CurrentTime = k_uptime_get();
        Messenger_ProcessQueue();
        RunUserLogic();
        k_msleep(1);
    }
#else
    while (true)
    {
        CurrentTime = k_uptime_get();
        Messenger_ProcessQueue();
        k_msleep(1);
    }
#endif
}

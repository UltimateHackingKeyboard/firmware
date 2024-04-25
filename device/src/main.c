#include "zephyr/storage/flash_map.h"
#include "keyboard/key_scanner.h"
#include "keyboard/leds.h"
#include "keyboard/oled/oled.h"
#include "keyboard/charger.h"
#include "keyboard/spi.h"
#include "keyboard/uart.h"
#include "bt_central_uart.h"
#include "bt_peripheral_uart.h"
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

int main(void) {
    printk("----------\n" DEVICE_NAME " started\n");

#if DEVICE_IS_UHK80_RIGHT
    flash_area_open(FLASH_AREA_ID(hardware_config_partition), &hardwareConfigArea);
    flash_area_open(FLASH_AREA_ID(user_config_partition), &userConfigArea);
#endif

#if !DEVICE_IS_UHK_DONGLE
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

#endif // !DEVICE_IS_UHK_DONGLE
    USB_EnableHid();

    bt_init();
    InitSettings();

#if DEVICE_IS_UHK80_LEFT
    InitPeripheralUart();
#endif

#if DEVICE_IS_UHK80_RIGHT
    HOGP_Enable();
    advertise_hid();
#endif

#if DEVICE_IS_UHK80_RIGHT || DEVICE_IS_UHK_DONGLE
    InitCentralUart();
#endif

#if DEVICE_IS_UHK80_RIGHT
    printk("Reading hardware config\n");
    flash_area_read(hardwareConfigArea, 0, HardwareConfigBuffer.buffer, HARDWARE_CONFIG_SIZE);
    printk("Reading user config\n");
    flash_area_read(userConfigArea, 0, StagingUserConfigBuffer.buffer, USER_CONFIG_SIZE);
    printk("Applying user config\n");
    UsbCommand_ApplyConfig();
    printk("User config applied\n");
    ShortcutParser_initialize();
    KeyIdParser_initialize();
    Macros_Initialize();
#endif

    HID_SendReportsThread();
}

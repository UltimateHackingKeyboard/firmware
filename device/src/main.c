#include "bt_hid.h"
#include "key_scanner.h"
#include "leds.h"
#include "oled.h"
#include "charger.h"
#include "spi.h"
#include "uart.h"
#include "central_uart.h"
#include "peripheral_uart.h"
#include "i2c.h"
#include "merge_sensor.h"
#include "shell.h"
#include "device.h"
#include "usb/usb.hpp"
#include <zephyr/drivers/gpio.h>

//const struct gpio_dt_spec testLed = GPIO_DT_SPEC_GET(DT_ALIAS(test_led), gpios);

int main(void) {
    printk("----------\n" DEVICE_NAME " started\n");

    // while (true) {
    //     gpio_pin_configure_dt(&testLed, GPIO_OUTPUT);
    //     gpio_pin_toggle_dt(&testLed);
    //     k_msleep(1000);
    // }

    InitUart();
    InitI2c();
    InitSpi();

    #ifdef DEVICE_HAS_OLED
    InitOled();
    #endif // DEVICE_HAS_OLED

    InitLeds();
    InitCharger();
    InitMergeSensor();
    InitKeyScanner();
    usb_init(true);
    bluetooth_init();

#if CONFIG_DEVICE_ID == DEVICE_ID_UHK80_LEFT
    InitPeripheralUart();
#elif CONFIG_DEVICE_ID == DEVICE_ID_UHK80_RIGHT
    InitCentralUart();
#endif

}
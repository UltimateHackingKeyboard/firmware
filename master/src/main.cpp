#include <zephyr/shell/shell.h>
extern "C"
{
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/usb/class/usb_hid.h>

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
#include "i2c.h"
#include "merge_sensor.h"
#include "shell.h"
}

#include "usb/usb.hpp"
#include "usb/keyboard_app.hpp"
#include "usb/mouse_app.hpp"
#include "usb/controls_app.hpp"
#include "usb/gamepad_app.hpp"
#include <zephyr/drivers/adc.h>
#include "device.h"

int main(void) {
    printk("----------\n" DEVICE_NAME " started\n");

    InitUart();
    InitI2c();
    k_mutex_init(&SpiMutex);
    InitOled();
    InitLeds();
    InitCharger();
    InitMergeSensor();

    usb_init(true);
    bluetooth_init();
    InitKeyScanner();

//  int blink_status = 0;

    for (;;) {
        sendUsbReports();

//      bluetooth_set_adv_led(&blink_status);
//      k_sleep(K_MSEC(ADV_LED_BLINK_INTERVAL));
        // Battery level simulation
        bas_notify();

        k_msleep(1);
    }
}

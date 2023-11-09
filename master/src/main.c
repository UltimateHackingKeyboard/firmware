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
#include "device.h"
#include "usb/usb.hpp"

int main(void) {
    printk("----------\n" DEVICE_NAME " started\n");

    InitUart();
    InitI2c();
    InitSpi();
    InitOled();
    InitLeds();
    InitCharger();
    InitMergeSensor();
    InitKeyScanner();
    usb_init(true);
    bluetooth_init();
}

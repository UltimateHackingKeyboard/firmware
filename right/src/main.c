#include "include/board/clock_config.h"
#include "init_peripherials.h"
#include "usb_composite_device.h"
#include "led_driver.h"

void main() {
    InitPeripherials();
    BOARD_BootClockRUN();
    LedDriver_EnableAllLeds();
    InitUsb();

    while (1);
}

#include "init_clock.h"
#include "init_peripherials.h"
#include "usb_composite_device.h"
#include "led_driver.h"

void main() {
    InitPeripherials();
    InitClock();
    LedDriver_InitAllLeds(1);
    UsbKeyboadTask();
    InitUsb();

    while (1){
        UsbKeyboadTask();
        asm("wfi");
    }
}

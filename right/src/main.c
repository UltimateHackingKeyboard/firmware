#include "init_clock.h"
#include "init_peripherials.h"
#include "usb_composite_device.h"
#include "led_driver.h"

void main() {
    InitPeripherials();
    InitClock();
    LedDriver_InitAllLeds(0);
    usbKeyboadTask();
    InitUsb();

    while (1){
    	usbKeyboadTask();
    	asm("wfi");
    }
}

#include "include/board/clock_config.h"
#include "include/board/board.h"
#include "include/board/pin_mux.h"
#include "usb_composite_device.h"

void main(void)
{
    BOARD_InitPins();
    BOARD_BootClockHSRUN();
    BOARD_InitDebugConsole();

    USB_DeviceApplicationInit();

    while (1) {
    }
}

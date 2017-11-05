#include "fsl_i2c.h"
#include "usb_protocol_handler.h"
#include "slave_scheduler.h"
#include "i2c_watchdog.h"

uint8_t UsbDebugInfo[USB_GENERIC_HID_OUT_BUFFER_LENGTH];

void UsbCommand_GetDebugInfo(void)
{
    *(uint32_t*)(UsbDebugInfo+1) = I2C_Watchdog;
    *(uint32_t*)(UsbDebugInfo+5) = I2cSchedulerCounter;
    *(uint32_t*)(UsbDebugInfo+9) = I2cWatchdog_OuterCounter;
    *(uint32_t*)(UsbDebugInfo+13) = I2cWatchdog_InnerCounter;

    memcpy(GenericHidOutBuffer, UsbDebugInfo, USB_GENERIC_HID_OUT_BUFFER_LENGTH);

/*    uint64_t ticks = microseconds_get_ticks();
    uint32_t microseconds = microseconds_convert_to_microseconds(ticks);
    uint32_t milliseconds = microseconds/1000;
    *(uint32_t*)(GenericHidOutBuffer+1) = ticks;
*/
}

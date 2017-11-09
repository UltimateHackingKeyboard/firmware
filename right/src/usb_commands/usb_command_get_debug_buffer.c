#include "fsl_i2c.h"
#include "usb_command_get_debug_buffer.h"
#include "usb_protocol_handler.h"
#include "slave_scheduler.h"
#include "i2c_watchdog.h"

uint8_t DebugBuffer[USB_GENERIC_HID_OUT_BUFFER_LENGTH];

void UsbCommand_GetDebugBuffer(void)
{
    SET_DEBUG_BUFFER_UINT32(1, I2C_Watchdog);
    SET_DEBUG_BUFFER_UINT32(5, I2cSlaveScheduler_Counter);
    SET_DEBUG_BUFFER_UINT32(9, I2cWatchdog_WatchCounter);
    SET_DEBUG_BUFFER_UINT32(13, I2cWatchdog_RecoveryCounter);

    memcpy(GenericHidOutBuffer, DebugBuffer, USB_GENERIC_HID_OUT_BUFFER_LENGTH);

/*    uint64_t ticks = microseconds_get_ticks();
    uint32_t microseconds = microseconds_convert_to_microseconds(ticks);
    uint32_t milliseconds = microseconds/1000;
    *(uint32_t*)(GenericHidOutBuffer+1) = ticks;
*/
}

#include <stdint.h>
#include <string.h>
#ifndef __ZEPHYR__
#include "i2c.h"
#include "i2c_watchdog.h"
#include "right_key_matrix.h"
#endif
#include "usb_command_get_debug_buffer.h"
#include "usb_protocol_handler.h"
#include "slave_scheduler.h"
#include "buffer.h"
#include "timer.h"
#include "usb_report_updater.h"

uint8_t DebugBuffer[USB_COMMAND_BUFFER_LENGTH];

void UsbCommand_GetDebugBuffer(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
#ifndef __ZEPHYR__
    SetBufferUint32(GenericHidInBuffer, 1, I2C_Watchdog);
    SetBufferUint32(GenericHidInBuffer, 5, I2cSlaveScheduler_Counter);
    SetBufferUint32(GenericHidInBuffer, 9, I2cWatchdog_WatchCounter);
    SetBufferUint32(GenericHidInBuffer, 13, I2cWatchdog_RecoveryCounter);
    SetBufferUint32(GenericHidInBuffer, 17, MatrixScanCounter);
    SetBufferUint32(GenericHidInBuffer, 21, UsbReportUpdateCounter);
#endif
    SetBufferUint32(GenericHidInBuffer, 25, Timer_GetCurrentTime());
}

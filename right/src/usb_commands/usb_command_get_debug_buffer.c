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
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "usb_interfaces/usb_interface_media_keyboard.h"
#include "usb_interfaces/usb_interface_system_keyboard.h"
#include "usb_interfaces/usb_interface_mouse.h"
#include "usb_interfaces/usb_interface_gamepad.h"

uint8_t DebugBuffer[USB_GENERIC_HID_IN_BUFFER_LENGTH];

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
#ifndef __ZEPHYR__
    SetBufferUint32(GenericHidInBuffer, 29, UsbGenericHidActionCounter);
    SetBufferUint32(GenericHidInBuffer, 33, UsbBasicKeyboardActionCounter);
    SetBufferUint32(GenericHidInBuffer, 37, UsbMediaKeyboardActionCounter);
    SetBufferUint32(GenericHidInBuffer, 41, UsbSystemKeyboardActionCounter);
    SetBufferUint32(GenericHidInBuffer, 45, UsbMouseActionCounter);
    SetBufferUint32(GenericHidInBuffer, 49, UsbGamepadActionCounter);
#endif
}

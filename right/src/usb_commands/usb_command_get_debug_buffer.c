#include "fsl_i2c.h"
#include "usb_command_get_debug_buffer.h"
#include "usb_protocol_handler.h"
#include "slave_scheduler.h"
#include "i2c_watchdog.h"
#include "buffer.h"
#include "timer.h"

uint8_t DebugBuffer[USB_GENERIC_HID_OUT_BUFFER_LENGTH];

void UsbCommand_GetDebugBuffer(void)
{
    SetDebugBufferUint32(1, I2C_Watchdog);
    SetDebugBufferUint32(5, I2cSlaveScheduler_Counter);
    SetDebugBufferUint32(9, I2cWatchdog_WatchCounter);
    SetDebugBufferUint32(13, I2cWatchdog_RecoveryCounter);
    SetDebugBufferUint32(40, CurrentTime);

    memcpy(GenericHidOutBuffer, DebugBuffer, USB_GENERIC_HID_OUT_BUFFER_LENGTH);
}

void SetDebugBufferUint8(uint32_t offset, uint8_t value)
{
    SetBufferUint8(DebugBuffer, offset, value);
}

void SetDebugBufferUint16(uint32_t offset, uint16_t value)
{
    SetBufferUint16(DebugBuffer, offset, value);
}

void SetDebugBufferUint32(uint32_t offset, uint32_t value)
{
    SetBufferUint32(DebugBuffer, offset, value);
}

void SetDebugBufferInt8(uint32_t offset, int8_t value)
{
    SetBufferInt8(DebugBuffer, offset, value);
}

void SetDebugBufferInt16(uint32_t offset, int16_t value)
{
    SetBufferInt16(DebugBuffer, offset, value);
}

void SetDebugBufferInt32(uint32_t offset, int32_t value)
{
    SetBufferInt32(DebugBuffer, offset, value);
}

void SetDebugBufferFloat(uint32_t offset, float value)
{
    *(float*)(DebugBuffer + offset) = value;
}

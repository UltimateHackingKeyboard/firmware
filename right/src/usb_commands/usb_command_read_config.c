#include "fsl_common.h"
#include "usb_commands/usb_command_read_config.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"

void UsbCommand_ReadConfig(bool isHardware)
{
    uint8_t length = GenericHidInBuffer[1];
    uint16_t offset = *(uint16_t*)(GenericHidInBuffer+2);

    if (length > USB_GENERIC_HID_OUT_BUFFER_LENGTH-1) {
        SetUsbStatusCode(UsbStatusCode_TransferConfig_LengthTooLarge);
        return;
    }

    uint8_t *buffer = isHardware ? HardwareConfigBuffer.buffer : ValidatedUserConfigBuffer.buffer;
    uint16_t bufferLength = isHardware ? HARDWARE_CONFIG_SIZE : USER_CONFIG_SIZE;

    if (offset + length > bufferLength) {
        SetUsbStatusCode(UsbStatusCode_TransferConfig_BufferOutOfBounds);
        return;
    }

    memcpy(GenericHidOutBuffer+1, buffer+offset, length);
}

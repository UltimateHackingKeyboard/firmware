#include "fsl_common.h"
#include "usb_commands/usb_command_read_config.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"

void UsbCommand_ReadConfig(bool isHardware)
{
    uint8_t length = GetUsbRxBufferUint8(1);
    uint16_t offset = GetUsbRxBufferUint16(2);

    if (length > USB_GENERIC_HID_OUT_BUFFER_LENGTH - USB_STATUS_CODE_SIZE) {
        SetUsbTxBufferUint8(0, UsbStatusCode_ReadConfig_LengthTooLarge);
        return;
    }

    uint8_t *buffer = isHardware ? HardwareConfigBuffer.buffer : ValidatedUserConfigBuffer.buffer;
    uint16_t bufferLength = isHardware ? HARDWARE_CONFIG_SIZE : USER_CONFIG_SIZE;

    if (offset + length > bufferLength) {
        SetUsbTxBufferUint8(0, UsbStatusCode_ReadConfig_BufferOutOfBounds);
        return;
    }

    memcpy(GenericHidOutBuffer + USB_STATUS_CODE_SIZE, buffer + offset, length);
}

#include "fsl_common.h"
#include "usb_commands/usb_command_write_config.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"

void UsbCommand_WriteConfig(config_buffer_id_t configBufferId)
{
    uint8_t length = GetUsbRxBufferUint8(1);
    uint16_t offset = GetUsbRxBufferUint16(2);
    const uint8_t paramsSize = USB_STATUS_CODE_SIZE + sizeof(length) + sizeof(offset);

    if (length > USB_GENERIC_HID_IN_BUFFER_LENGTH - paramsSize) {
        SetUsbTxBufferUint8(0, UsbStatusCode_WriteConfig_LengthTooLarge);
        return;
    }

    uint8_t *buffer = ConfigBufferIdToConfigBuffer(configBufferId)->buffer;
    uint16_t bufferLength = ConfigBufferIdToBufferSize(configBufferId);

    if (offset + length > bufferLength) {
        SetUsbTxBufferUint8(0, UsbStatusCode_WriteConfig_BufferOutOfBounds);
        return;
    }

    memcpy(buffer + offset, GenericHidOutBuffer + paramsSize, length);
}

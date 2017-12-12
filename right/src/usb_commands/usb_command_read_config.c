#include "fsl_common.h"
#include "usb_commands/usb_command_read_config.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"

void UsbCommand_ReadConfig()
{
    config_buffer_id_t configBufferId = GetUsbRxBufferUint8(1);
    uint8_t length = GetUsbRxBufferUint8(2);
    uint16_t offset = GetUsbRxBufferUint16(3);

    if (!IsConfigBufferIdValid(configBufferId)) {
        SetUsbTxBufferUint8(0, UsbStatusCode_ReadConfig_InvalidConfigBufferId);
    }

    if (length > USB_GENERIC_HID_OUT_BUFFER_LENGTH - USB_STATUS_CODE_SIZE) {
        SetUsbTxBufferUint8(0, UsbStatusCode_ReadConfig_LengthTooLarge);
        return;
    }

    config_buffer_t *buffer = ConfigBufferIdToConfigBuffer(configBufferId);
    uint16_t bufferLength = ConfigBufferIdToBufferSize(configBufferId);

    if (offset + length > bufferLength) {
        SetUsbTxBufferUint8(0, UsbStatusCode_ReadConfig_BufferOutOfBounds);
        return;
    }

    memcpy(GenericHidOutBuffer + USB_STATUS_CODE_SIZE, buffer->buffer + offset, length);
}

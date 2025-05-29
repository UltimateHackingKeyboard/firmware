#include <string.h>
#ifndef __ZEPHYR__
#include "fsl_common.h"
#endif
#include "usb_commands/usb_command_write_config.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"

void UsbCommand_WriteConfig(config_buffer_id_t configBufferId, const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    uint8_t length = GetUsbRxBufferUint8(1);
    uint16_t offset = GetUsbRxBufferUint16(2);
    const uint8_t paramsSize = USB_STATUS_CODE_SIZE + sizeof(length) + sizeof(offset);

    if (length > USB_GENERIC_HID_IN_BUFFER_LENGTH - paramsSize) {
        SetUsbTxBufferUint8(0, UsbStatusCode_WriteConfig_LengthTooLarge);
        return;
    }

    config_buffer_t* bufferHead = ConfigBufferIdToConfigBuffer(configBufferId);
    uint8_t *buffer = bufferHead->buffer;
    uint16_t bufferLength = ConfigBufferIdToBufferSize(configBufferId);

    if (offset + length > bufferLength) {
        SetUsbTxBufferUint8(0, UsbStatusCode_WriteConfig_BufferOutOfBounds);
        return;
    }

    bufferHead->isValid = false;
    memcpy(buffer + offset, GenericHidOutBuffer + paramsSize, length);
}

#include "usb_commands/usb_command_validate_buffer_crc.h"
#include "usb_protocol_handler.h"
#include "config_parser/config_globals.h"
#include "crc16.h"

void UsbCommand_ValidateBufferCrc(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    uint8_t bufferId = GetUsbRxBufferUint8(1);
    uint16_t expectedSize = GetUsbRxBufferUint16(2);
    uint16_t expectedCrc = GetUsbRxBufferUint16(4);

    if (!IsConfigBufferIdValid(bufferId)) {
        SetUsbTxBufferUint8(0, UsbStatusCode_ValidateBufferCrc_InvalidBufferId);
        return;
    }

    config_buffer_t *buffer = ConfigBufferIdToConfigBuffer(bufferId);
    uint16_t bufferSize = ConfigBufferIdToBufferSize(bufferId);

    if (expectedSize > bufferSize) {
        SetUsbTxBufferUint8(0, UsbStatusCode_ValidateBufferCrc_SizeOutOfBounds);
        return;
    }

    crc16_data_t crcData;
    crc16_init(&crcData);
    crc16_update(&crcData, buffer->buffer, expectedSize);

    uint16_t computedCrc;
    crc16_finalize(&crcData, &computedCrc);

    if (computedCrc != expectedCrc) {
        SetUsbTxBufferUint8(0, UsbStatusCode_ValidateBufferCrc_CrcMismatch);
        return;
    }
}

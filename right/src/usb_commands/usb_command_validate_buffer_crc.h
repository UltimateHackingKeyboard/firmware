#ifndef __USB_COMMAND_VALIDATE_BUFFER_CRC_H__
#define __USB_COMMAND_VALIDATE_BUFFER_CRC_H__

// Includes:

    #include <stdint.h>

// Typedefs:

    typedef enum {
        UsbStatusCode_ValidateBufferCrc_CrcMismatch     = 2,
        UsbStatusCode_ValidateBufferCrc_InvalidBufferId  = 3,
        UsbStatusCode_ValidateBufferCrc_SizeOutOfBounds  = 4,
    } usb_status_code_validate_buffer_crc_t;

// Functions:

    void UsbCommand_ValidateBufferCrc(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif

#ifndef __USB_COMMAND_WRITE_CONFIG_H__
#define __USB_COMMAND_WRITE_CONFIG_H__

// Includes:

    #include "config_parser/config_globals.h"

// Typedefs:

    typedef enum {
        UsbStatusCode_WriteConfig_LengthTooLarge    = 2,
        UsbStatusCode_WriteConfig_BufferOutOfBounds = 3,
    } usb_status_code_write_config_t;

// Functions:

    void UsbCommand_WriteConfig(config_buffer_id_t configBufferId, const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif

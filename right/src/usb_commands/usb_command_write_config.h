#ifndef __USB_COMMAND_WRITE_CONFIG_H__
#define __USB_COMMAND_WRITE_CONFIG_H__

// Functions:

    void UsbCommand_WriteConfig(bool isHardware);

// Typedefs:

    typedef enum {
        UsbStatusCode_WriteConfig_LengthTooLarge    = 2,
        UsbStatusCode_WriteConfig_BufferOutOfBounds = 3,
    } usb_status_code_write_config_t;

#endif

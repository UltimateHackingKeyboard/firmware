#ifndef __USB_COMMAND_READ_CONFIG_H__
#define __USB_COMMAND_READ_CONFIG_H__

// Functions:

    void UsbCommand_ReadConfig(bool isHardware);

// Typedefs:

    typedef enum {
        UsbStatusCode_ReadConfig_LengthTooLarge    = 2,
        UsbStatusCode_ReadConfig_BufferOutOfBounds = 3,
    } usb_status_code_read_config_t;

#endif

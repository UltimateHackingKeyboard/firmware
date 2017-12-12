#ifndef __USB_COMMAND_READ_CONFIG_H__
#define __USB_COMMAND_READ_CONFIG_H__

// Functions:

    void UsbCommand_ReadConfig();

// Typedefs:

    typedef enum {
        UsbStatusCode_ReadConfig_InvalidConfigBufferId = 2,
        UsbStatusCode_ReadConfig_LengthTooLarge        = 3,
        UsbStatusCode_ReadConfig_BufferOutOfBounds     = 4,
    } usb_status_code_read_config_t;

#endif

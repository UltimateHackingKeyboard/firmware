#ifndef __USB_COMMAND_GET_MODULE_PROPERTIES_H__
#define __USB_COMMAND_GET_MODULE_PROPERTIES_H__

// Functions:

    void UsbCommand_GetModuleProperties();

// Typedefs:

    typedef enum {
        UsbStatusCode_ReadConfig_InvalidModuleSlotId = 2,
    } usb_status_code_get_module_properties_t;

#endif

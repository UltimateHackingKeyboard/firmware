#ifndef __USB_COMMAND_GET_MODULE_PROPERTY_H__
#define __USB_COMMAND_GET_MODULE_PROPERTY_H__

// Functions:

    void UsbCommand_GetModuleProperty();

// Typedefs:

    typedef enum {
        UsbStatusCode_GetModuleProperty_InvalidModuleSlotId = 2,
    } usb_status_code_get_module_property_t;

#endif

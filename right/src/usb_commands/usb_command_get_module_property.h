#ifndef __USB_COMMAND_GET_MODULE_PROPERTY_H__
#define __USB_COMMAND_GET_MODULE_PROPERTY_H__

// Includes:

    #include <stdint.h>

// Functions:

    void UsbCommand_GetModuleProperty(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

// Typedefs:

    typedef enum {
        ModulePropertyId_VersionNumbers = 0,
        ModulePropertyId_GitTag = 1,
        ModulePropertyId_GitRepo = 2,
        ModulePropertyId_FirmwareChecksum = 3,
    } module_property_id_t;

    typedef enum {
        UsbStatusCode_GetModuleProperty_InvalidModuleSlotId = 2,
    } usb_status_code_get_module_property_t;

#endif

#ifndef __SLAVE_DRIVER_TOUCHPAD_MODULE_H__
#define __SLAVE_DRIVER_TOUCHPAD_MODULE_H__

// Includes:

    #include "fsl_common.h"
    #include "crc16.h"
    #include "versions.h"
    #include "slot.h"
    #include "usb_interfaces/usb_interface_mouse.h"

// Typedefs:

    typedef enum {
        TouchpadDriverId_Singleton,
    } touchpad_driver_id_t;

// Variables:

    extern usb_mouse_report_t TouchpadUsbMouseReport;

// Functions:

    void TouchpadDriver_Init(uint8_t uhkModuleDriverId);
    status_t TouchpadDriver_Update(uint8_t uhkModuleDriverId);
    void TouchpadDriver_Disconnect(uint8_t uhkModuleDriverId);

#endif

#ifndef __USB_COMMAND_GET_SLAVE_I2C_ERRORS_H__
#define __USB_COMMAND_GET_SLAVE_I2C_ERRORS_H__

// Functions:

    void UsbCommand_GetSlaveI2cErrors();

// Typedefs:

    typedef enum {
        UsbStatusCode_GetModuleProperty_InvalidSlaveId = 2,
    } usb_status_code_get_slave_i2c_errors_t;

#endif

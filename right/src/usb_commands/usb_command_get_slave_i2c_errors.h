#ifndef __USB_COMMAND_GET_SLAVE_I2C_ERRORS_H__
#define __USB_COMMAND_GET_SLAVE_I2C_ERRORS_H__

// Includes:

    #include <stdint.h>

// Functions:

    void UsbCommand_GetSlaveI2cErrors(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

// Typedefs:

    typedef enum {
        UsbStatusCode_GetModuleProperty_InvalidSlaveId = 2,
    } usb_status_code_get_slave_i2c_errors_t;

#endif

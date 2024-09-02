#ifndef __KEYBOARD_I2C_H__
#define __KEYBOARD_I2C_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>
    #include "keyboard/i2c_compatibility.h"

// Functions:

    extern void InitKeyboardI2c(void);
    status_t KeyboardI2c_MasterTransferNonBlocking(i2c_master_transfer_t *transfer);

#endif // __KEYBOARD_I2C_H__

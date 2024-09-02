#ifndef __ZEPHYR_I2C_H__
#define __ZEPHYR_I2C_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>
    #include "keyboard/i2c_compatibility.h"

// Functions:

    extern void InitZephyrI2c(void);
    status_t ZephyrI2c_MasterTransferNonBlocking(i2c_master_transfer_t *transfer);

#endif // __ZEPHYR_I2C_H__

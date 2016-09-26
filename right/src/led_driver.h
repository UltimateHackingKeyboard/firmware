#ifndef __LED_DRIVER_H__
#define __LED_DRIVER_H__

// Includes:

#include "fsl_gpio.h"
#include "fsl_port.h"

    #include "fsl_i2c.h"
    #include "i2c.h"
    #include "i2c_addresses.h"

// Macros:

    extern void LedDriver_WriteBuffer(uint8_t txBuffer[], uint8_t size);
    extern void LedDriver_WriteRegister(uint8_t reg, uint8_t val);
    extern void LedDriver_EnableAllLeds();

#endif

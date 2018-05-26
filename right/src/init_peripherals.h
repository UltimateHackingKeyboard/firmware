#ifndef __INIT_PERIPHERALS_H__
#define __INIT_PERIPHERALS_H__

// Includes

    #include "fsl_common.h"

// Typedefs

typedef struct {
    GPIO_Type *sdaGpio;
    PORT_Type *sdaPort;
    uint32_t sdaPin;

    GPIO_Type *sclGpio;
    PORT_Type *sclPort;
    uint32_t sclPin;
} i2c_bus_t;

// Variables:

    extern bool IsBusPalOn;
    extern volatile uint32_t I2cMainBusRequestedBaudRateBps;
    extern volatile uint32_t I2cMainBusActualBaudRateBps;

// Functions:

    void InitPeripherals(void);
    void ReinitI2cMainBus(void);

#endif

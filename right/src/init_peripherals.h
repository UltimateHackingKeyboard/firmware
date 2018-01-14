#ifndef __INIT_PERIPHERALS_H__
#define __INIT_PERIPHERALS_H__

// Includes

    #include "fsl_common.h"

// Variables:

    extern bool IsBusPalOn;
    extern uint32_t I2cMainBusRequestedBaudRateBps;
    extern uint32_t I2cMainBusActualBaudRateBps;

// Functions:

    void InitPeripherals(void);
    void ReinitI2cMainBus(void);

#endif

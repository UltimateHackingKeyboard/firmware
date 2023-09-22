#ifndef __INIT_PERIPHERALS_H__
#define __INIT_PERIPHERALS_H__

// Includes:

    #include "fsl_common.h"

// Typedefs:

    typedef struct {
        clock_name_t clockSrc;
        I2C_Type *baseAddr;
        uint16_t mux;

        clock_ip_name_t sdaClock;
        GPIO_Type *sdaGpio;
        PORT_Type *sdaPort;
        uint32_t sdaPin;

        clock_ip_name_t sclClock;
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
    void ChangeI2cBaudRate(uint32_t i2cBaudRate);

#endif

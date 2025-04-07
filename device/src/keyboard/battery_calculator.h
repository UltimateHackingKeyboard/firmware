#ifndef __BATTERY_CALCULATOR_H__
#define __BATTERY_CALCULATOR_H__

// Includes:

    #include "charger.h"
    #include <zephyr/drivers/adc.h>
    #include <inttypes.h>
    #include <stdbool.h>

// Macros:

// Typedefs:

    typedef struct {
        uint8_t keyCount;
        uint32_t brightnessSum;
        uint16_t loadedVoltage;
        uint16_t unloadedVoltage;
        uint16_t loadedCurrent;
        uint16_t unloadedCurrent;
    } resistance_reference_t;

// Variables:

// Functions:

    uint16_t BatteryCalculator_CalculateUnloadedVoltage(uint16_t rawVoltage);
    void BatteryCalculator_RunTests(void);

#endif // __BATTERY_CALCULATOR_H__

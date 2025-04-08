#ifndef __BATTERY_PERCENT_CALCULATOR_H__
#define __BATTERY_PERCENT_CALCULATOR_H__

// Includes:

    #include "charger.h"
    #include <zephyr/drivers/adc.h>
    #include <inttypes.h>
    #include <stdbool.h>

// Macros:

// Typedefs:

// Variables:

// Functions:

    uint16_t BatteryCalculator_CalculatePercent(uint16_t correctedVoltage);

#endif // __BATTERY_CALCULATOR_H__

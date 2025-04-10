#ifndef __BATTERY_WINDOW_CALCULATOR_H__
#define __BATTERY_WINDOW_CALCULATOR_H__

// Includes:

    #include "charger.h"
    #include <zephyr/drivers/adc.h>
    #include <inttypes.h>
    #include <stdbool.h>
    #include "debug.h"

// Macros:

#if DEBUG_BATTERY_TESTING
    #define BATTERY_CALCULATOR_AVERAGE_ENABLED false
#else
    #define BATTERY_CALCULATOR_AVERAGE_ENABLED true
#endif

// Typedefs:

// Variables:

// Functions:

    uint16_t BatteryCalculator_CalculateWindowAverageVoltage(uint16_t rawVoltage);

#endif // __BATTERY_CALCULATOR_H__

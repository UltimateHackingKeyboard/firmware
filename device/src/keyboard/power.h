#ifndef __POWER_H__
#define __POWER_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>

// Variables:

    extern bool RunningOnBattery;

// Functions:

    void Power_ReportPowerState(uint8_t level, uint32_t ma);

#endif // __POWER_H__


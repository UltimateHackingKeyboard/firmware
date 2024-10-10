#ifndef __CHARGER_H__
#define __CHARGER_H__

// Includes:

    #include "shared/attributes.h"
    #include <zephyr/drivers/adc.h>
    #include <inttypes.h>
    #include <stdbool.h>

// Macros:

    #define VOLTAGE_DIVIDER_MULTIPLIER 2

// Typedefs:

    typedef struct {
        uint16_t batteryVoltage;
        uint8_t batteryPercentage;
        bool batteryPresent;
        bool batteryCharging;
        bool powered;
    } ATTR_PACKED battery_state_t;

// Variables:

    extern const struct gpio_dt_spec chargerEnDt;
    extern const struct gpio_dt_spec chargerStatDt;
    extern const struct adc_dt_spec adc_channel;

    extern bool UsbPowered;
    extern bool RunningOnBattery;
    extern bool RightRunningOnBattery;

// Functions:

    void InitCharger(void);
    void Charger_PrintState();
    void Charger_UpdateBatteryState();

#endif // __CHARGER_H__

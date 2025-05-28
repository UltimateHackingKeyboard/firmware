#ifndef __CHARGER_H__
#define __CHARGER_H__

// Includes:

    #include "shared/attributes.h"
    #include <zephyr/drivers/adc.h>
    #include <inttypes.h>
    #include <stdbool.h>
    #include "debug.h"

// Macros:

    #define VOLTAGE_DIVIDER_MULTIPLIER 2


    #define CHARGER_UPDATE_PERIOD 60*1000

    #define CHARGER_STAT_PERIOD 700
    #define CHARGER_STABILIZATION_PERIOD 500

// Typedefs:

    typedef struct {
        uint16_t batteryVoltage;
        uint8_t batteryPercentage;
        bool batteryPresent;
        bool batteryCharging;
        bool powersaving;
        bool powered;
    } ATTR_PACKED battery_state_t;

// Variables:

    extern const struct gpio_dt_spec chargerEnDt;
    extern const struct gpio_dt_spec chargerStatDt;
    extern const struct adc_dt_spec adc_channel;

    extern bool UsbPowered;
    extern bool RunningOnBattery;
    extern bool BatteryIsCharging;
    extern bool RightRunningOnBattery;
    extern bool Charger_ChargingEnabled;

// Functions:

    void InitCharger(void);
    void InitCharger_Min(void);
    void Charger_PrintState();
    void Charger_UpdateBatteryState();

    bool Charger_EnableCharging(bool enabled);

    bool Charger_ShouldRemainInDepletedMode(bool checkVoltage);
    bool Charger_ShouldEnterDepletedMode();

#endif // __CHARGER_H__

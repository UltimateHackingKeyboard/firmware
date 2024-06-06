#ifndef __CHARGER_H__
#define __CHARGER_H__

// Includes:

    #include <zephyr/drivers/adc.h>

// Macros:

    #define VOLTAGE_DIVIDER_MULTIPLIER 2

// Variables:

    extern const struct gpio_dt_spec chargerEnDt;
    extern const struct gpio_dt_spec chargerStatDt;
    extern const struct adc_dt_spec adc_channel;

// Functions:

    void InitCharger(void);
    void Charger_PrintState();
    void Charger_UpdateBatteryState();

#endif // __CHARGER_H__

#ifndef __CHARGER_H__
#define __CHARGER_H__

// Includes:

    #include <zephyr/drivers/adc.h>


// Variables:

    extern const struct gpio_dt_spec chargerEnDt;
    extern const struct gpio_dt_spec chargerStatDt;

    extern const struct adc_dt_spec adc_channels[2];

// Functions:

    extern void InitCharger(void);

#endif

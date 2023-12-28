#ifndef __LEDS_H__
#define __LEDS_H__

// Variables:

    extern const struct gpio_dt_spec ledsCsDt;
    extern const struct gpio_dt_spec ledsSdbDt;

// Functions:

    extern void InitLeds(void);

#endif // __LEDS_H__

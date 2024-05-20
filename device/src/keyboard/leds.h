#ifndef __LEDS_H__
#define __LEDS_H__

// Macros:

    #define LED_DRIVER_LED_COUNT_MAX 255
// Variables:

    extern uint8_t Uhk80LedDriverValues[LED_DRIVER_LED_COUNT_MAX];
    extern const struct gpio_dt_spec ledsCsDt;
    extern const struct gpio_dt_spec ledsSdbDt;

// Functions:

    extern void Uhk80_UpdateLeds();
    extern void InitLeds(void);

#endif // __LEDS_H__

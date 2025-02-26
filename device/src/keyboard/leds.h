#ifndef __LEDS_H__
#define __LEDS_H__

// Includes

    #include <stdint.h>

// Macros:

    #define UHK80_LED_DRIVER_LED_COUNT_MAX 255
// Variables:

    extern uint8_t Uhk80LedDriverValues[UHK80_LED_DRIVER_LED_COUNT_MAX];
    extern const struct gpio_dt_spec ledsCsDt;
    extern const struct gpio_dt_spec ledsSdbDt;

// Functions:

    extern void Uhk80_UpdateLeds();
    extern void InitLeds(void);
    extern void InitLeds_Min(void);
    extern void Leds_BlinkSfjl(uint16_t time);
    extern void UpdateLedAudioRegisters(uint8_t spreadSpectrum, uint8_t phaseDelay, uint8_t pwmFrequency);

#endif // __LEDS_H__

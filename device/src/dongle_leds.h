#include "device.h"
#if !defined __DONGLE_LEDS_H__ && DEVICE_IS_UHK_DONGLE
#define __DONGLE_LEDS_H__

// Includes:

    #include <zephyr/drivers/pwm.h>
    #include <stdbool.h>
    #include <stdint.h>

// Typedefs:

// Functions:

    extern void DongleLeds_Set(uint8_t r, uint8_t g, uint8_t b);
    extern void DongleLeds_Update(void);
    extern void set_dongle_led(const struct pwm_dt_spec *device, uint8_t percentage);

// Variables:

    extern const struct pwm_dt_spec red_pwm_led;
    extern const struct pwm_dt_spec green_pwm_led;
    extern const struct pwm_dt_spec blue_pwm_led;

#else
    #include "stubs.h"
#endif // __DONGLE_LEDS_H__

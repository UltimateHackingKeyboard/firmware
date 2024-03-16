#if defined __DONGLE_LEDS_H__ && DEVICE_IS_UHK_DONGLE
#define __DONGLE_LEDS_H__

// Includes:

    #include <zephyr/drivers/pwm.h>

// Functions:

    extern void set_dongle_led(const struct pwm_dt_spec *device, uint8_t percentage);

// Variables:

    extern const struct pwm_dt_spec red_pwm_led;
    extern const struct pwm_dt_spec green_pwm_led;
    extern const struct pwm_dt_spec blue_pwm_led;

#endif // __DONGLE_LEDS_H__

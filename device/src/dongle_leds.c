#include "device.h"
#if DEVICE_IS_UHK_DONGLE
#include "dongle_leds.h"

const struct pwm_dt_spec red_pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(red_pwm_led));
const struct pwm_dt_spec green_pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(green_pwm_led));
const struct pwm_dt_spec blue_pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(blue_pwm_led));

void set_dongle_led(const struct pwm_dt_spec *device, uint8_t percentage) {
    pwm_set_pulse_dt(device, percentage * device->period / 100);
}

#endif // DEVICE_IS_UHK_DONGLE

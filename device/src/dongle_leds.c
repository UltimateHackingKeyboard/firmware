#include "connections.h"
#include "device.h"
#if DEVICE_IS_UHK_DONGLE
#include "dongle_leds.h"
#include "device_state.h"
#include "settings.h"

const struct pwm_dt_spec red_pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(red_pwm_led));
const struct pwm_dt_spec green_pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(green_pwm_led));
const struct pwm_dt_spec blue_pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(blue_pwm_led));

// There is also the following zero led.
// const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0_green), gpios);
// gpio_pin_configure_dt(&led0, GPIO_OUTPUT);
//     gpio_pin_set_dt(&led0, true);
//     k_sleep(K_MSEC(1000));
//     gpio_pin_set_dt(&led0, false);

void set_dongle_led(const struct pwm_dt_spec *device, uint8_t percentage) {
    pwm_set_pulse_dt(device, percentage * device->period / 100);
}


static bool authorizationFailed = false;

void DongleLeds_Set(uint8_t r, uint8_t g, uint8_t b) {
    set_dongle_led(&red_pwm_led, r);
    set_dongle_led(&green_pwm_led, g);
    set_dongle_led(&blue_pwm_led, b);
}

void DongleLeds_SetAuthorizationFailed(bool failed) {
    if (authorizationFailed != failed) {
        authorizationFailed = failed;
        DongleLeds_Update();
    }
}

void DongleLeds_Update(void) {
    bool isConnected = Connections_IsReady(ConnectionId_NusServerRight);
    bool isPaired = RightAddressIsSet;

    if (isConnected && !DongleStandby) {
        // connected and receiving: green
        DongleLeds_Set(0, 100, 0);
    } else if (isConnected) {
        // connected in standby: blue
        DongleLeds_Set(0, 100, 100);
    } else if (authorizationFailed) {
        // connected, but not authorized: red
        DongleLeds_Set(100, 0, 0);
    } else if (isPaired) {
        // trying to connect: violet
        DongleLeds_Set(100, 0, 70);
    } else {
        // disconnected: red
        DongleLeds_Set(100, 0, 0);
    }
}

#endif // DEVICE_IS_UHK_DONGLE

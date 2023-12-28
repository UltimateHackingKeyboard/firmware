#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

const struct gpio_dt_spec testLed = GPIO_DT_SPEC_GET(DT_ALIAS(test_led), gpios);

int main(void) {
    // printk("----------\n" DEVICE_NAME " started\n");

    while (true) {
        gpio_pin_configure_dt(&testLed, GPIO_OUTPUT);
        gpio_pin_toggle_dt(&testLed);
        k_msleep(1000);
    }
}

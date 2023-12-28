#include "device.h"

#ifdef DEVICE_HAS_OLED

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "keyboard/oled.h"
#include "spi.h"

// Thread definitions

#define THREAD_STACK_SIZE 1000
#define THREAD_PRIORITY 5

static K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
static struct k_thread thread_data;

// OLED GPIOs

const struct gpio_dt_spec oledEn = GPIO_DT_SPEC_GET(DT_ALIAS(oled_en), gpios);
const struct gpio_dt_spec oledResetDt = GPIO_DT_SPEC_GET(DT_ALIAS(oled_reset), gpios);
const struct gpio_dt_spec oledCsDt = GPIO_DT_SPEC_GET(DT_ALIAS(oled_cs), gpios);
const struct gpio_dt_spec oledA0Dt = GPIO_DT_SPEC_GET(DT_ALIAS(oled_a0), gpios);

void setOledCs(bool state)
{
    gpio_pin_set_dt(&oledCsDt, state);
}

void setA0(bool state)
{
    gpio_pin_set_dt(&oledA0Dt, state);
}

static uint32_t counter = 0;
static uint8_t pixel = 1;

void oledUpdater() {
    while (true) {
        k_mutex_lock(&SpiMutex, K_FOREVER);

        setA0(false);
        setOledCs(false);
        writeSpi(0xaf);
        setOledCs(true);

        setA0(false);
        setOledCs(false);
        writeSpi(0x81);
        writeSpi(0xff);
        setOledCs(true);

        setA0(true);
        setOledCs(false);
        writeSpi(pixel ? 0xff : 0x00);
        setOledCs(true);

        k_mutex_unlock(&SpiMutex);

        if (counter++ > 19) {
            pixel = !pixel;
            counter = 0;
        }
        k_msleep(1);
    }
}

void InitOled(void) {
    gpio_pin_configure_dt(&oledEn, GPIO_OUTPUT);
    gpio_pin_set_dt(&oledEn, true);

    gpio_pin_configure_dt(&oledResetDt, GPIO_OUTPUT);
    gpio_pin_set_dt(&oledResetDt, false);
    k_msleep(1);
    gpio_pin_set_dt(&oledResetDt, true);

    gpio_pin_configure_dt(&oledCsDt, GPIO_OUTPUT);
    gpio_pin_configure_dt(&oledA0Dt, GPIO_OUTPUT);

    k_thread_create(
        &thread_data, stack_area,
        K_THREAD_STACK_SIZEOF(stack_area),
        oledUpdater,
        NULL, NULL, NULL,
        THREAD_PRIORITY, 0, K_NO_WAIT
    );
    k_thread_name_set(&thread_data, "oled_updater");

}

#endif // DEVICE_HAS_OLED

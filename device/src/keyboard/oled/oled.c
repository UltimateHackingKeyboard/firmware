#include "device.h"
#include "oled.h"
#include "oled_buffer.h"

#ifdef DEVICE_HAS_OLED

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "keyboard/oled/oled.h"
#include "keyboard/spi.h"

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

void oledCommand1(bool A0, uint8_t b1) {
    setA0(A0);
    setOledCs(true);
    writeSpi(b1);
    setOledCs(false);
}

void oledCommand2(bool A0, uint8_t b1, uint8_t b2) {
    setA0(A0);
    setOledCs(true);
    writeSpi(b1);
    writeSpi(b2);
    setOledCs(false);
}


void oledUpdater() {
    k_mutex_lock(&SpiMutex, K_FOREVER);
    oledCommand1(0, 0xaf); // turn the panel on
    oledCommand2(0, 0x81, 0xff); //set maximum contrast
    k_mutex_unlock(&SpiMutex);

    while (true) {
        k_mutex_lock(&SpiMutex, K_FOREVER);

        for (uint16_t atPixel = 0; atPixel < DISPLAY_HEIGHT*DISPLAY_WIDTH; atPixel += 2) {
            #define PIXEL(AT) (OledBuffer[DISPLAY_HEIGHT-(AT)/DISPLAY_WIDTH][DISPLAY_WIDTH-(AT)%DISPLAY_WIDTH]);

            uint8_t firstPixel = PIXEL(atPixel);
            uint8_t secondPixel = PIXEL(atPixel+1);
            uint8_t upper = firstPixel & 0xf0;
            uint8_t lower = secondPixel >> 4;

            oledCommand1(1, upper | lower); //write pixel data
        }

        k_mutex_unlock(&SpiMutex);

        while (!OledBuffer_NeedsRedraw) {
            k_msleep(10);
        }
        OledBuffer_NeedsRedraw = false;
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

    OledBuffer_Init();
}

#endif // DEVICE_HAS_OLED

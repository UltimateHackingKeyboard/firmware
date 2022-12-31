#include <zephyr/kernel.h>
#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <drivers/spi.h>

#define LedPagePrefix 0b01010000

static struct spi_config spiConf = {
    .frequency = 400000U,
    .operation = (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB)
};

uint8_t buf[] = {1};
const struct spi_buf spiBuf[] = {
    {
        .buf = &buf,
        .len = 1,
    }
};

const struct spi_buf_set spiBufSet = {
    .buffers = spiBuf,
    .count = 1,
};

const struct device *spi0_dev = DEVICE_DT_GET(DT_NODELABEL(spi1));
static const struct gpio_dt_spec oledCsDt = GPIO_DT_SPEC_GET(DT_ALIAS(oled_cs), gpios);
static const struct gpio_dt_spec ledsCsDt = GPIO_DT_SPEC_GET(DT_ALIAS(leds_cs), gpios);
static const struct gpio_dt_spec oledA0Dt = GPIO_DT_SPEC_GET(DT_ALIAS(oled_a0), gpios);

void setLedsCs(bool state)
{
    gpio_pin_set_dt(&ledsCsDt, state);
}

void setOledCs(bool state)
{
    gpio_pin_set_dt(&oledCsDt, state);
}

void setA0(bool state)
{
    gpio_pin_set_dt(&oledA0Dt, state);
}

void writeSpi(uint8_t data)
{
    buf[0] = data;
    spi_write(spi0_dev, &spiConf, &spiBufSet);
}

void main(void)
{
    gpio_pin_configure_dt(&oledCsDt, GPIO_OUTPUT);
    gpio_pin_configure_dt(&ledsCsDt, GPIO_OUTPUT);
    gpio_pin_configure_dt(&oledA0Dt, GPIO_OUTPUT);

    uint32_t counter = 0;
    bool pixel = 1;
    while (true) {
//      printk("spi send: a\n");
        setA0(false);
        setOledCs(false);
        writeSpi(0xaf);
        setOledCs(true);

        setA0(true);
        setOledCs(false);
        writeSpi(pixel ? 0xff : 0x00);
        setOledCs(true);

        setLedsCs(false);
        writeSpi(LedPagePrefix | 2);
        writeSpi(0x00);
        writeSpi(0b00001001);
        setLedsCs(true);

        setLedsCs(false);
        writeSpi(LedPagePrefix | 2);
        writeSpi(0x01);
        writeSpi(0xff);
        setLedsCs(true);

        setLedsCs(false);
        writeSpi(LedPagePrefix | 0);
        writeSpi(0x00);
        for (int i=0; i<255; i++) {
            writeSpi(0xff);
        }
        setLedsCs(true);

        setLedsCs(false);
        writeSpi(LedPagePrefix | 1);
        writeSpi(0x00);
        for (int i=0; i<255; i++) {
            writeSpi(0xff);
        }
        setLedsCs(true);

//        k_msleep(1);

        if (counter++ > 19) {
            pixel = !pixel;
            counter = 0;
        }
    }
}

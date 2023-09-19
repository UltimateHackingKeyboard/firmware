#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/shell/shell.h>

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
static const struct gpio_dt_spec oledEn = GPIO_DT_SPEC_GET(DT_ALIAS(oled_en), gpios);
static const struct gpio_dt_spec oledResetDt = GPIO_DT_SPEC_GET(DT_ALIAS(oled_reset), gpios);
static const struct gpio_dt_spec oledCsDt = GPIO_DT_SPEC_GET(DT_ALIAS(oled_cs), gpios);
static const struct gpio_dt_spec ledsCsDt = GPIO_DT_SPEC_GET(DT_ALIAS(leds_cs), gpios);
static const struct gpio_dt_spec ledsSdb = GPIO_DT_SPEC_GET(DT_ALIAS(leds_sdb), gpios);
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

static struct gpio_dt_spec rows[] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(row1), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(row2), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(row3), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(row4), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(row5), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(row6), gpios),
};

static struct gpio_dt_spec cols[] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(col1), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col2), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col3), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col4), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col5), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col6), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col7), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col8), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col9), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(col10), gpios),
};

char volatile c = 0;
void main(void)
{
    printk("----------\nUHK 80 right half started\n");

    gpio_pin_configure_dt(&ledsSdb, GPIO_OUTPUT);
    gpio_pin_set_dt(&ledsSdb, true);

    gpio_pin_configure_dt(&oledEn, GPIO_OUTPUT);
    gpio_pin_set_dt(&oledEn, true);

    gpio_pin_configure_dt(&oledResetDt, GPIO_OUTPUT);
    gpio_pin_set_dt(&oledResetDt, false);
    k_msleep(1);
    gpio_pin_set_dt(&oledResetDt, true);

//  struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));
    gpio_pin_configure_dt(&oledCsDt, GPIO_OUTPUT);
    gpio_pin_configure_dt(&ledsCsDt, GPIO_OUTPUT);
    gpio_pin_configure_dt(&oledA0Dt, GPIO_OUTPUT);

    for (uint8_t rowId=0; rowId<6; rowId++) {
        gpio_pin_configure_dt(&rows[rowId], GPIO_OUTPUT);
    }
    for (uint8_t colId=0; colId<10; colId++) {
        gpio_pin_configure_dt(&cols[colId], GPIO_INPUT);
    }

    uint32_t counter = 0;
    bool pixel = 1;
    while (true) {
        c = 0;
        for (uint8_t rowId=0; rowId<6; rowId++) {
            gpio_pin_set_dt(&rows[rowId], 1);
            for (uint8_t colId=0; colId<10; colId++) {
                if (gpio_pin_get_dt(&cols[colId])) {
                    c = 1;
                    printk("SW%c%c ", rowId+'1', colId+'1');
                }
            }
            gpio_pin_set_dt(&rows[rowId], 0);
        }

//      uart_poll_out(uart_dev, 'a');

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
            writeSpi(c?0xff:0);
        }
        setLedsCs(true);

        setLedsCs(false);
        writeSpi(LedPagePrefix | 1);
        writeSpi(0x00);
        for (int i=0; i<255; i++) {
            writeSpi(c?0xff:0);
        }
        setLedsCs(true);

//        k_msleep(1);

        if (counter++ > 19) {
            pixel = !pixel;
            counter = 0;
        }
    }
}

#include <zephyr/kernel.h>
#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

#include <drivers/spi.h>

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

void writeSpi(uint8_t data)
{
	buf[0] = data;
	spi_write(spi0_dev, &spiConf, &spiBufSet);
}

void main(void)
{
	while (true) {
//		printk("spi send: a\n");
		for (uint8_t i=0; i<4; i++) {
			writeSpi(i);
		}
        k_msleep(1);
	}
}

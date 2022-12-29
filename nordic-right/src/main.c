#include <zephyr/kernel.h>
#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

#include <drivers/spi.h>

static struct spi_config spi_conf = {
	.frequency = 400000U,
	.operation = (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB)
};

void main(void)
{
	uint8_t buf[] = {1, 2, 3};
    const struct spi_buf spiBuf[] = {
		{
			.buf = &buf,
			.len = 3,
    	}
	};

    const struct spi_buf_set spiBufSet = {
        .buffers = spiBuf,
        .count = 1,
	};
	const struct device *spi0_dev = DEVICE_DT_GET(DT_NODELABEL(spi1));

	while (true) {
		printk("spi send: a\n");
		spi_write(spi0_dev, &spi_conf, &spiBufSet);
//        k_msleep(1);
	}
}

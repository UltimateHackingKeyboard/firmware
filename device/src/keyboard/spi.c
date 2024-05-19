#include "keyboard/spi.h"
#include <string.h>

struct k_mutex SpiMutex;

const struct device *spi0_dev = DEVICE_DT_GET(DT_NODELABEL(spi1));

static struct spi_config spiConf = {
    .frequency = 32000000U,
    .operation = (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB)
};


uint8_t buf[1] = {1};
struct spi_buf spiBuf[] = {
    {
        .buf = &buf,
        .len = 1,
    }
};

const struct spi_buf_set spiBufSet = {
    .buffers = spiBuf,
    .count = 1,
};

void writeSpi(uint8_t data)
{
    buf[0] = data;
    spiBuf[0].buf = buf;
    spiBuf[0].len = 1;
    spi_write(spi0_dev, &spiConf, &spiBufSet);
}

void writeSpi2(uint8_t* data, uint8_t len)
{
    spiBuf[0].buf = data;
    spiBuf[0].len = len;
    spi_write(spi0_dev, &spiConf, &spiBufSet);
}

void InitSpi(void) {
    k_mutex_init(&SpiMutex);
}

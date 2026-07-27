#include "keyboard/spi.h"
#include <string.h>
#include <zephyr/pm/device.h>

struct k_mutex SpiMutex;

const struct device *spi0_dev = DEVICE_DT_GET(DT_NODELABEL(spi1));

static struct spi_config spiConf = {
    .frequency = 32000000U,
    .operation = (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB)
};

// The bus is shared by the LED driver and the OLED, both of which write through
// writeSpi/writeSpi2 under SpiMutex, so all of the state below is guarded by that mutex.
static bool spiSuspended = false;
static bool ledsIdle = false;
static bool displayIdle = false;

// Call with SpiMutex held.
static void resumeSpiIfSuspended(void) {
    if (spiSuspended) {
        pm_device_action_run(spi0_dev, PM_DEVICE_ACTION_RESUME);
        spiSuspended = false;
    }
}

// Suspend the bus once both consumers are idle; the next transfer resumes it lazily.
static void updateSpiPower(void) {
    k_mutex_lock(&SpiMutex, K_FOREVER);
    if (ledsIdle && displayIdle && !spiSuspended) {
        if (pm_device_action_run(spi0_dev, PM_DEVICE_ACTION_SUSPEND) == 0) {
            spiSuspended = true;
        } else {
            printk("Failed to suspend the SPI bus\n");
        }
    }
    k_mutex_unlock(&SpiMutex);
}

void Spi_SetLedsIdle(bool idle) {
    ledsIdle = idle;
    updateSpiPower();
}

void Spi_SetDisplayIdle(bool idle) {
    displayIdle = idle;
    updateSpiPower();
}

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
    resumeSpiIfSuspended();
    buf[0] = data;
    spiBuf[0].buf = buf;
    spiBuf[0].len = 1;
    spi_write(spi0_dev, &spiConf, &spiBufSet);
}

void writeSpi2(uint8_t* data, uint8_t len)
{
    resumeSpiIfSuspended();
    spiBuf[0].buf = data;
    spiBuf[0].len = len;
    spi_write(spi0_dev, &spiConf, &spiBufSet);
}

void InitSpi(void) {
    k_mutex_init(&SpiMutex);
}

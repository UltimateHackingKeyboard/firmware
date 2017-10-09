#include "debug_over_spi.h"
#include "config.h"
#include "fsl_gpio.h"

#ifdef DEBUG_OVER_SPI

#define EXAMPLE_SPI_MASTER (SPI0)
#define EXAMPLE_SPI_MASTER_SOURCE_CLOCK (kCLOCK_BusClk)

#define BUFFER_SIZE (64)
static uint8_t srcBuff[BUFFER_SIZE];

static spi_transfer_t xfer = {0};
static spi_master_config_t userConfig;
spi_master_handle_t handle;

static volatile bool masterFinished = true;

#endif

static void masterCallback(SPI_Type *base, spi_master_handle_t *masterHandle, status_t status, void *userData)
{
#ifdef DEBUG_OVER_SPI
    masterFinished = true;
#endif
}

void DebugOverSpi_Init(void)
{
#ifdef DEBUG_OVER_SPI

    CLOCK_EnableClock(DEBUG_OVER_SPI_MOSI_CLOCK);
    CLOCK_EnableClock(DEBUG_OVER_SPI_SCK_CLOCK);

    PORT_SetPinMux(DEBUG_OVER_SPI_MOSI_PORT, DEBUG_OVER_SPI_MOSI_PIN, kPORT_MuxAlt3);
    PORT_SetPinMux(DEBUG_OVER_SPI_SCK_PORT, DEBUG_OVER_SPI_SCK_PIN, kPORT_MuxAlt3);

    GPIO_PinInit(DEBUG_OVER_SPI_MOSI_GPIO, DEBUG_OVER_SPI_MOSI_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 0});
    GPIO_PinInit(DEBUG_OVER_SPI_SCK_GPIO, DEBUG_OVER_SPI_SCK_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 0});

    GPIO_SetPinsOutput(DEBUG_OVER_SPI_MOSI_GPIO, 1U << DEBUG_OVER_SPI_MOSI_PIN);
    GPIO_SetPinsOutput(DEBUG_OVER_SPI_SCK_GPIO, 1U << DEBUG_OVER_SPI_SCK_PIN);

    SPI_MasterGetDefaultConfig(&userConfig);
    uint32_t srcFreq = CLOCK_GetFreq(EXAMPLE_SPI_MASTER_SOURCE_CLOCK);
    SPI_MasterInit(EXAMPLE_SPI_MASTER, &userConfig, srcFreq);
    SPI_MasterTransferCreateHandle(EXAMPLE_SPI_MASTER, &handle, masterCallback, NULL);
#endif
}

void DebugOverSpi_Send(uint8_t *tx, uint8_t len)
{
#ifdef DEBUG_OVER_SPI
    if (masterFinished) {
        masterFinished = false;
        memcpy(srcBuff, tx, MIN(BUFFER_SIZE, len));
        xfer.txData = srcBuff;
        xfer.dataSize = len;
        SPI_MasterTransferNonBlocking(EXAMPLE_SPI_MASTER, &handle, &xfer);
    }
#endif
}

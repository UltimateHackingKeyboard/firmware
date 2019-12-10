#include "fsl_gpio.h"
#include "fsl_port.h"
#include "fsl_spi.h"
#include "trackball.h"
#include "test_led.h"

pointer_delta_t Trackball_PointerDelta;

#define TX_SIZE 2
#define RX_SIZE 1

uint8_t txBuffer[TX_SIZE];
uint8_t rxBuffer[RX_SIZE];
spi_master_handle_t handle;

void trackballUpdate(SPI_Type *base, spi_master_handle_t *masterHandle, status_t status, void *userData)
{
//    Trackball_PointerDelta.x++;
//    Trackball_PointerDelta.y++;
}

void Trackball_Init(void)
{
    CLOCK_EnableClock(TRACKBALL_SHTDWN_CLOCK);
    PORT_SetPinConfig(TRACKBALL_SHTDWN_PORT, TRACKBALL_SHTDWN_PIN, &(port_pin_config_t){.pullSelect=kPORT_PullDown, .mux=kPORT_MuxAsGpio});
    GPIO_PinInit(TRACKBALL_SHTDWN_GPIO, TRACKBALL_SHTDWN_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 0});
    GPIO_WritePinOutput(TRACKBALL_SHTDWN_GPIO, TRACKBALL_SHTDWN_PIN, 0);

    CLOCK_EnableClock(TRACKBALL_NCS_CLOCK);
    PORT_SetPinMux(TRACKBALL_NCS_PORT, TRACKBALL_NCS_PIN, kPORT_MuxAsGpio);
    GPIO_WritePinOutput(TRACKBALL_NCS_GPIO, TRACKBALL_NCS_PIN, 0);

    CLOCK_EnableClock(TRACKBALL_MOSI_CLOCK);
    PORT_SetPinMux(TRACKBALL_MOSI_PORT, TRACKBALL_MOSI_PIN, kPORT_MuxAlt3);

    CLOCK_EnableClock(TRACKBALL_MISO_CLOCK);
    PORT_SetPinMux(TRACKBALL_MISO_PORT, TRACKBALL_MOSI_PIN, kPORT_MuxAlt3);

    CLOCK_EnableClock(TRACKBALL_SCK_CLOCK);
    PORT_SetPinMux(TRACKBALL_SCK_PORT, TRACKBALL_MOSI_PIN, kPORT_MuxAlt3);

    uint32_t srcFreq = 0;
    spi_master_config_t userConfig;
    // userConfig.enableStopInWaitMode = false;
    // userConfig.polarity = kSPI_ClockPolarityActiveHigh;
    // userConfig.phase = kSPI_ClockPhaseFirstEdge;
    // userConfig.direction = kSPI_MsbFirst;
    // userConfig.dataMode = kSPI_8BitMode;
    // userConfig.txWatermark = kSPI_TxFifoOneHalfEmpty;
    // userConfig.rxWatermark = kSPI_RxFifoOneHalfFull;
    // userConfig.pinMode = kSPI_PinModeNormal;
    // userConfig.outputMode = kSPI_SlaveSelectAutomaticOutput;
    // userConfig.baudRate_Bps = 500000U;
    SPI_MasterGetDefaultConfig(&userConfig);
    srcFreq = CLOCK_GetFreq(TRACKBALL_SPI_MASTER_SOURCE_CLOCK);
    SPI_MasterInit(TRACKBALL_SPI_MASTER, &userConfig, srcFreq);

    spi_transfer_t xfer = {0};
    xfer.txData = txBuffer;
    xfer.dataSize = TX_SIZE;
    SPI_MasterTransferCreateHandle(TRACKBALL_SPI_MASTER, &handle, trackballUpdate, NULL);
    SPI_MasterTransferNonBlocking(TRACKBALL_SPI_MASTER, &handle, &xfer);
}

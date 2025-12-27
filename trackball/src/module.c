#include "fsl_gpio.h"
#include "fsl_port.h"
#include "fsl_spi.h"
#include "module.h"
#include "module/uart.h"
#include "module/init_peripherals.h"

#define TRACKBALL_SHTDWN_PORT PORTA
#define TRACKBALL_SHTDWN_GPIO GPIOA
#define TRACKBALL_SHTDWN_IRQ PORTA_IRQn
#define TRACKBALL_SHTDWN_CLOCK kCLOCK_PortA
#define TRACKBALL_SHTDWN_PIN 4

#define TRACKBALL_NCS_PORT PORTB
#define TRACKBALL_NCS_GPIO GPIOB
#define TRACKBALL_NCS_IRQ PORTB_IRQn
#define TRACKBALL_NCS_CLOCK kCLOCK_PortB
#define TRACKBALL_NCS_PIN 1

#define TRACKBALL_MOSI_PORT PORTA
#define TRACKBALL_MOSI_GPIO GPIOA
#define TRACKBALL_MOSI_IRQ PORTA_IRQn
#define TRACKBALL_MOSI_CLOCK kCLOCK_PortA
#define TRACKBALL_MOSI_PIN 7

#define TRACKBALL_MISO_PORT PORTA
#define TRACKBALL_MISO_GPIO GPIOA
#define TRACKBALL_MISO_IRQ PORTA_IRQn
#define TRACKBALL_MISO_CLOCK kCLOCK_PortA
#define TRACKBALL_MISO_PIN 6

#define TRACKBALL_SCK_PORT PORTB
#define TRACKBALL_SCK_GPIO GPIOB
#define TRACKBALL_SCK_IRQ PORTB_IRQn
#define TRACKBALL_SCK_CLOCK kCLOCK_PortB
#define TRACKBALL_SCK_PIN 0

#define TRACKBALL_SPI_MASTER SPI0
#define TRACKBALL_SPI_MASTER_SOURCE_CLOCK kCLOCK_BusClk

pointer_delta_t PointerDelta;

key_vector_t KeyVector = {
    .itemNum = KEYBOARD_VECTOR_ITEMS_NUM,
    .items = (key_vector_pin_t[]) {
        {PORTA, GPIOA, kCLOCK_PortA, 3}, // left button
        {PORTA, GPIOA, kCLOCK_PortA, 12}, // right button
    },
    .keyStates = {0}
};

#define BUFFER_SIZE 2
#define MOTION_BIT (1<<7)

typedef enum {
    ModulePhase_SetResolution,
    ModulePhase_SetRestRate3,
    ModulePhase_Initialized,
    ModulePhase_ProcessMotion,
    ModulePhase_ProcessDeltaY,
    ModulePhase_ProcessDeltaX,
} module_phase_t;

module_phase_t modulePhase = ModulePhase_SetResolution;

uint8_t txBufferPowerUpReset[] = {0xba, 0x5a};
uint8_t txSetResolution[] = {0x91, 0b10000000};
uint8_t txSetRestRate3[] = {0x18, 0x09};
uint8_t txBufferGetMotion[] = {0x02, 0x00};
uint8_t txBufferGetDeltaY[] = {0x03, 0x00};
uint8_t txBufferGetDeltaX[] = {0x04, 0x00};

uint8_t rxBuffer[BUFFER_SIZE];
spi_master_handle_t handle;
spi_transfer_t xfer = {0};

void tx(uint8_t *txBuff)
{
    GPIO_PinWrite(TRACKBALL_NCS_GPIO, TRACKBALL_NCS_PIN, 1);
    GPIO_PinWrite(TRACKBALL_NCS_GPIO, TRACKBALL_NCS_PIN, 0);
    xfer.txData = txBuff;
    SPI_MasterTransferNonBlocking(TRACKBALL_SPI_MASTER, &handle, &xfer);
}

void trackballUpdate(SPI_Type *base, spi_master_handle_t *masterHandle, status_t status, void *userData)
{
    switch (modulePhase) {
        case ModulePhase_SetResolution:
            tx(txSetResolution);
            modulePhase = ModulePhase_SetRestRate3;
            break;
        case ModulePhase_SetRestRate3:
            tx(txSetRestRate3);
            modulePhase = ModulePhase_Initialized;
            break;
        case ModulePhase_Initialized:
            tx(txBufferGetMotion);
            modulePhase = ModulePhase_ProcessMotion;
            break;
        case ModulePhase_ProcessMotion: ;
            uint8_t motion = rxBuffer[1];
            bool isMoved = motion & MOTION_BIT;
            if (isMoved) {
                tx(txBufferGetDeltaY);
                modulePhase = ModulePhase_ProcessDeltaY;
            } else {
                tx(txBufferGetMotion);
            }
            break;
        case ModulePhase_ProcessDeltaY: ;
            int8_t deltaY = (int8_t)rxBuffer[1];
            PointerDelta.x += deltaY; // This is correct given the sensor orientation.
            tx(txBufferGetDeltaX);
            modulePhase = ModulePhase_ProcessDeltaX;
            break;
        case ModulePhase_ProcessDeltaX: ;
            int8_t deltaX = (int8_t)rxBuffer[1];
            PointerDelta.y += deltaX; // This is correct given the sensor orientation.
            tx(txBufferGetMotion);
            modulePhase = ModulePhase_ProcessMotion;
            if (MODULE_OVER_UART) {
                ModuleUart_RequestKeyStatesUpdate();
            }
            break;
    }
}

void Trackball_Init(void)
{
    CLOCK_EnableClock(TRACKBALL_SHTDWN_CLOCK);
    PORT_SetPinMux(TRACKBALL_SHTDWN_PORT, TRACKBALL_SHTDWN_PIN, kPORT_MuxAsGpio);
    GPIO_PinWrite(TRACKBALL_SHTDWN_GPIO, TRACKBALL_SHTDWN_PIN, 0);

    CLOCK_EnableClock(TRACKBALL_NCS_CLOCK);
    PORT_SetPinMux(TRACKBALL_NCS_PORT, TRACKBALL_NCS_PIN, kPORT_MuxAsGpio);

    CLOCK_EnableClock(TRACKBALL_MOSI_CLOCK);
    PORT_SetPinMux(TRACKBALL_MOSI_PORT, TRACKBALL_MOSI_PIN, kPORT_MuxAlt3);

    CLOCK_EnableClock(TRACKBALL_MISO_CLOCK);
    PORT_SetPinMux(TRACKBALL_MISO_PORT, TRACKBALL_MISO_PIN, kPORT_MuxAlt3);

    CLOCK_EnableClock(TRACKBALL_SCK_CLOCK);
    PORT_SetPinMux(TRACKBALL_SCK_PORT, TRACKBALL_SCK_PIN, kPORT_MuxAlt3);

    uint32_t srcFreq = 0;
    spi_master_config_t userConfig;
    SPI_MasterGetDefaultConfig(&userConfig);
    userConfig.polarity = kSPI_ClockPolarityActiveLow;
    userConfig.phase = kSPI_ClockPhaseSecondEdge;
    userConfig.baudRate_Bps = 100000U;
    srcFreq = CLOCK_GetFreq(TRACKBALL_SPI_MASTER_SOURCE_CLOCK);
    SPI_MasterInit(TRACKBALL_SPI_MASTER, &userConfig, srcFreq);
    SPI_MasterTransferCreateHandle(TRACKBALL_SPI_MASTER, &handle, trackballUpdate, NULL);
    xfer.rxData = rxBuffer;
    xfer.dataSize = BUFFER_SIZE;
    tx(txBufferPowerUpReset);
}

void Module_Init(void)
{
    KeyVector_Init(&KeyVector);
    Trackball_Init();
}

void Module_Loop(void)
{
}

void Module_ModuleSpecificCommand(module_specific_command_t command)
{
}

void Module_OnScan(void)
{
}

#include "fsl_i2c.h"
#include "include/board/clock_config.h"
#include "include/board/board.h"
#include "include/board/pin_mux.h"
#include "usb_composite_device.h"
#include "i2c.h"
#include "main.h"
#include "fsl_common.h"
#include "fsl_port.h"

void tx(uint8_t txBuffer[], uint8_t size)
{
    i2c_master_transfer_t masterXfer;
    masterXfer.slaveAddress = LEFT_LED_DRIVER_ADDRESS_7BIT;
    masterXfer.direction = kI2C_Write;
    masterXfer.subaddress = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data = txBuffer;
    masterXfer.dataSize = size;
    masterXfer.flags = kI2C_TransferDefaultFlag;
    I2C_MasterTransferBlocking(EXAMPLE_I2C_MASTER_BASEADDR, &masterXfer);
    masterXfer.slaveAddress = RIGHT_LED_DRIVER_ADDRESS_7BIT;
    I2C_MasterTransferBlocking(EXAMPLE_I2C_MASTER_BASEADDR, &masterXfer);
}

void write(uint8_t reg, uint8_t val)
{
    uint8_t txBuffer[] = {0, 0};
    txBuffer[0] = reg;
    txBuffer[1] = val;
    tx(txBuffer, sizeof(txBuffer));
}

void InitLedDisplay()
{
    PORT_SetPinMux(PORTA, 2U, kPORT_MuxAsGpio);

    gpio_pin_config_t led_config = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = 0,
    };

    GPIO_PinInit(GPIOA, 2U, &led_config);
    GPIO_SetPinsOutput(GPIOA,   0 << 2U);

    write(0xfd, 0x0b); // point to page 9
    write(0x0a, 0x01); // set shutdown mode to normal
    write(0xfd, 0x00); // point to page 0

    uint8_t i;
    for (i=0x00; i<=0x11; i++) {
        write(i, 0xff);
    }
    for (i=0x12; i<=0x23; i++) {
        write(i, 0x00);
    }
    for (i=0x24; i<=0xb3; i++) {
        write(i, 0xff);
    }
}

void main() {
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();

    i2c_master_config_t masterConfig;
    uint32_t sourceClock;
    I2C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Bps = I2C_BAUD_RATE;
    sourceClock = CLOCK_GetFreq(I2C_MASTER_CLK_SRC);
    I2C_MasterInit(EXAMPLE_I2C_MASTER_BASEADDR, &masterConfig, sourceClock);

    InitLedDisplay();
    InitUsb();

    while (1);
}

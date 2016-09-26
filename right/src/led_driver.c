#include "led_driver.h"

void LedDriver_WriteBuffer(uint8_t txBuffer[], uint8_t size)
{
    i2c_master_transfer_t masterXfer;
    masterXfer.slaveAddress = LEFT_LED_DRIVER_ADDRESS_7BIT;
    masterXfer.direction = kI2C_Write;
    masterXfer.subaddress = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data = txBuffer;
    masterXfer.dataSize = size;
    masterXfer.flags = kI2C_TransferDefaultFlag;
    I2C_MasterTransferBlocking(I2C_BASEADDR_MAIN_BUS, &masterXfer);
    masterXfer.slaveAddress = RIGHT_LED_DRIVER_ADDRESS_7BIT;
    I2C_MasterTransferBlocking(I2C_BASEADDR_MAIN_BUS, &masterXfer);
}

void LedDriver_WriteRegister(uint8_t reg, uint8_t val)
{
    uint8_t txBuffer[] = {0, 0};
    txBuffer[0] = reg;
    txBuffer[1] = val;
    LedDriver_WriteBuffer(txBuffer, sizeof(txBuffer));
}

void LedDriver_EnableAllLeds()
{
    PORT_SetPinMux(PORTA, 2U, kPORT_MuxAsGpio);

    gpio_pin_config_t led_config = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = 0,
    };

    GPIO_PinInit(GPIOA, 2U, &led_config);
    GPIO_SetPinsOutput(GPIOA,   0 << 2U);

    LedDriver_WriteRegister(0xfd, 0x0b); // point to page 9
    LedDriver_WriteRegister(0x0a, 0x01); // set shutdown mode to normal
    LedDriver_WriteRegister(0xfd, 0x00); // point to page 0

    uint8_t i;
    for (i=0x00; i<=0x11; i++) {
        LedDriver_WriteRegister(i, 0xff);
    }
    for (i=0x12; i<=0x23; i++) {
        LedDriver_WriteRegister(i, 0x00);
    }
    for (i=0x24; i<=0xb3; i++) {
        LedDriver_WriteRegister(i, 0xff);
    }
}

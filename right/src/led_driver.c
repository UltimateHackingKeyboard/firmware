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
    uint8_t txBuffer[] = {reg, val};
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

    LedDriver_WriteRegister(LED_DRIVER_REGISTER_FRAME, LED_DRIVER_FRAME_FUNCTION);
    LedDriver_WriteRegister(LED_DRIVER_REGISTER_SHUTDOWN, SHUTDOWN_MODE_NORMAL);
    LedDriver_WriteRegister(LED_DRIVER_REGISTER_FRAME, LED_DRIVER_FRAME_1);

    uint8_t i;
    for (i=FRAME_REGISTER_LED_CONTROL_FIRST; i<=FRAME_REGISTER_LED_CONTROL_LAST; i++) {
        LedDriver_WriteRegister(i, 0xff);
    }
    for (i=FRAME_REGISTER_BLINK_CONTROL_FIRST; i<=FRAME_REGISTER_BLINK_CONTROL_LAST; i++) {
        LedDriver_WriteRegister(i, 0x00);
    }
    for (i=FRAME_REGISTER_PWM_FIRST; i<=FRAME_REGISTER_PWM_LAST; i++) {
        LedDriver_WriteRegister(i, 0xff);
    }
}

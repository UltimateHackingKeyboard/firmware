#include "led_driver.h"

void LedDriver_WriteBuffer(uint8_t i2cAddress, uint8_t buffer[], uint8_t size)
{
    i2c_master_transfer_t masterXfer;
    masterXfer.slaveAddress = i2cAddress;
    masterXfer.direction = kI2C_Write;
    masterXfer.subaddress = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data = buffer;
    masterXfer.dataSize = size;
    masterXfer.flags = kI2C_TransferDefaultFlag;
    I2C_MasterTransferBlocking(I2C_BASEADDR_MAIN_BUS, &masterXfer);
}

void LedDriver_WriteRegister(uint8_t i2cAddress, uint8_t reg, uint8_t val)
{
    uint8_t buffer[] = {reg, val};
    LedDriver_WriteBuffer(i2cAddress, buffer, sizeof(buffer));
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

    uint8_t ledDriverAddresses[] = {I2C_ADDRESS_LED_DRIVER_LEFT, I2C_ADDRESS_LED_DRIVER_RIGHT};

    for (uint8_t addressId=0; addressId<sizeof(ledDriverAddresses); addressId++) {
        uint8_t address = ledDriverAddresses[addressId];
        LedDriver_WriteRegister(address, LED_DRIVER_REGISTER_FRAME, LED_DRIVER_FRAME_FUNCTION);
        LedDriver_WriteRegister(address, LED_DRIVER_REGISTER_SHUTDOWN, SHUTDOWN_MODE_NORMAL);
        LedDriver_WriteRegister(address, LED_DRIVER_REGISTER_FRAME, LED_DRIVER_FRAME_1);

        uint8_t i;
        for (i=FRAME_REGISTER_LED_CONTROL_FIRST; i<=FRAME_REGISTER_LED_CONTROL_LAST; i++) {
            LedDriver_WriteRegister(address, i, 0xff);
        }
        for (i=FRAME_REGISTER_BLINK_CONTROL_FIRST; i<=FRAME_REGISTER_BLINK_CONTROL_LAST; i++) {
            LedDriver_WriteRegister(address, i, 0x00);
        }
        for (i=FRAME_REGISTER_PWM_FIRST; i<=FRAME_REGISTER_PWM_LAST; i++) {
            LedDriver_WriteRegister(address, i, 0xff);
        }
    }
}

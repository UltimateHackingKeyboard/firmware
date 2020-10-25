#include "fsl_common.h"
#include "fsl_port.h"
#include "module/test_led.h"
#include "init_peripherals.h"
#include "i2c_addresses.h"
#include "fsl_i2c.h"
#include "fsl_clock.h"
#include "module/i2c.h"
#include "module/led_pwm.h"
#include "slave_protocol_handler.h"
#include "module/i2c_watchdog.h"
#include "module.h"

i2c_slave_config_t slaveConfig;
i2c_slave_handle_t slaveHandle;

uint8_t userData;
uint8_t rxMessagePos;
uint8_t dosBuffer[2];

static void i2cSlaveCallback(I2C_Type *base, i2c_slave_transfer_t *xfer, void *userDataArg)
{
    dosBuffer[0] = xfer->event;
    dosBuffer[1] = userData;

    switch (xfer->event) {
        case kI2C_SlaveTransmitEvent:
            SlaveTxHandler();
            xfer->data = (uint8_t*)&TxMessage;
            xfer->dataSize = TxMessage.length + I2C_MESSAGE_HEADER_LENGTH;
            break;
        case kI2C_SlaveAddressMatchEvent:
            rxMessagePos = 0;
            break;
        case kI2C_SlaveReceiveEvent:
            ((uint8_t*)&RxMessage)[rxMessagePos++] = userData;
            if (RxMessage.length == rxMessagePos-I2C_MESSAGE_HEADER_LENGTH) {
                SlaveRxHandler();
            }
            break;
        default:
            break;
    }
}

void initInterruptPriorities(void)
{
    NVIC_SetPriority(I2C0_IRQn, 1);
    NVIC_SetPriority(TPM1_IRQn, 1);
    NVIC_SetPriority(SPI0_IRQn, 1);
}

void initI2c(void)
{
    port_pin_config_t pinConfig = {
        .pullSelect = kPORT_PullUp,
    };

    CLOCK_EnableClock(I2C_BUS_SDA_CLOCK);
    CLOCK_EnableClock(I2C_BUS_SCL_CLOCK);

    pinConfig.mux = I2C_BUS_MUX;
    PORT_SetPinConfig(I2C_BUS_SDA_PORT, I2C_BUS_SDA_PIN, &pinConfig);
    PORT_SetPinConfig(I2C_BUS_SCL_PORT, I2C_BUS_SCL_PIN, &pinConfig);

    I2C_SlaveGetDefaultConfig(&slaveConfig);
    slaveConfig.slaveAddress = I2C_ADDRESS_MODULE_FIRMWARE;
#ifdef CPU_MKL17Z32VFM4
    I2C_SlaveInit(I2C_BUS_BASEADDR, &slaveConfig, I2C_BUS_BAUD_RATE);
#else
    I2C_SlaveInit(I2C_BUS_BASEADDR, &slaveConfig);
#endif
    I2C_SlaveTransferCreateHandle(I2C_BUS_BASEADDR, &slaveHandle, i2cSlaveCallback, &userData);
    I2C_SlaveTransferNonBlocking(I2C_BUS_BASEADDR, &slaveHandle, kI2C_SlaveAddressMatchEvent);
}

void InitLedDriver(void)
{
    CLOCK_EnableClock(LED_DRIVER_SDB_CLOCK);
    PORT_SetPinMux(LED_DRIVER_SDB_PORT, LED_DRIVER_SDB_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(LED_DRIVER_SDB_GPIO, LED_DRIVER_SDB_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 0});
    GPIO_WritePinOutput(LED_DRIVER_SDB_GPIO, LED_DRIVER_SDB_PIN, 1);
}

void InitPeripherals(void)
{
    initInterruptPriorities();
    InitLedDriver();
    TestLed_Init();
    LedPwm_Init();
    initI2c();
}

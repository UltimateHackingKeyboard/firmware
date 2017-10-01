#include "fsl_common.h"
#include "fsl_port.h"
#include "test_led.h"
#include "init_peripherals.h"
#include "i2c_addresses.h"
#include "fsl_i2c.h"
#include "fsl_clock.h"
#include "i2c.h"
#include "led_pwm.h"
#include "slave_protocol_handler.h"
#include "i2c_watchdog.h"

uint8_t byteIn;
uint8_t rxMessagePos;
i2c_slave_transfer_event_t prevEvent;

static void i2cSlaveCallback(I2C_Type *base, i2c_slave_transfer_t *xfer, void *userData)
{
    switch (xfer->event) {
        case kI2C_SlaveTransmitEvent:
            SlaveTxHandler();
            xfer->data = (uint8_t*)&TxMessage;
            xfer->dataSize = TxMessage.length + I2C_MESSAGE_HEADER_LENGTH;
            break;
        case kI2C_SlaveReceiveEvent:
            if (prevEvent == kI2C_SlaveReceiveEvent) {
                ((uint8_t*)&RxMessage)[rxMessagePos++] = byteIn;
            } else {
                rxMessagePos = 0;
                memset(&RxMessage, 0, I2C_MESSAGE_MAX_TOTAL_LENGTH);
            }

            xfer->data = (uint8_t*)&byteIn;
            xfer->dataSize = 1;
            break;
        case kI2C_SlaveCompletionEvent:
            if (prevEvent == kI2C_SlaveReceiveEvent) {
                ((uint8_t*)&RxMessage)[rxMessagePos] = byteIn;
                RxMessage.length = rxMessagePos - I2C_MESSAGE_HEADER_LENGTH;
                SlaveRxHandler();
            }
            break;
        default:
            break;
    }

    prevEvent = xfer->event;
}

void InitInterruptPriorities(void)
{
    NVIC_SetPriority(I2C0_IRQn, 1);
    NVIC_SetPriority(TPM1_IRQn, 1);
}

void InitI2c(void) {
    port_pin_config_t pinConfig = {
        .pullSelect = kPORT_PullUp,
    };

    CLOCK_EnableClock(I2C_BUS_SDA_CLOCK);
    CLOCK_EnableClock(I2C_BUS_SCL_CLOCK);

    pinConfig.mux = I2C_BUS_MUX;
    PORT_SetPinConfig(I2C_BUS_SDA_PORT, I2C_BUS_SDA_PIN, &pinConfig);
    PORT_SetPinConfig(I2C_BUS_SCL_PORT, I2C_BUS_SCL_PIN, &pinConfig);

    i2c_slave_config_t slaveConfig;
    i2c_slave_handle_t slaveHandle;

    I2C_SlaveGetDefaultConfig(&slaveConfig);
    slaveConfig.slaveAddress = I2C_ADDRESS_LEFT_KEYBOARD_HALF_FIRMWARE;
    I2C_SlaveInit(I2C_BUS_BASEADDR, &slaveConfig);
    I2C_SlaveTransferCreateHandle(I2C_BUS_BASEADDR, &slaveHandle, i2cSlaveCallback, NULL);
    slaveHandle.eventMask |= kI2C_SlaveCompletionEvent;
    I2C_SlaveTransferNonBlocking(I2C_BUS_BASEADDR, &slaveHandle, kI2C_SlaveCompletionEvent);
}

void InitI2cV2(void) {
    port_pin_config_t pinConfig = {
        .pullSelect = kPORT_PullUp,
    };

    CLOCK_EnableClock(I2C_BUS_SDA_CLOCK);
    CLOCK_EnableClock(I2C_BUS_SCL_CLOCK);

    pinConfig.mux = I2C_BUS_MUX;
    PORT_SetPinConfig(I2C_BUS_SDA_PORT, I2C_BUS_SDA_PIN, &pinConfig);
    PORT_SetPinConfig(I2C_BUS_SCL_PORT, I2C_BUS_SCL_PIN, &pinConfig);

    i2c_slave_config_t slaveConfig;
    i2c_slave_handle_t slaveHandle;

    I2C_SlaveGetDefaultConfig(&slaveConfig);
    slaveConfig.slaveAddress = I2C_ADDRESS_LEFT_KEYBOARD_HALF_FIRMWARE;
    I2C_SlaveInit(I2C_BUS_BASEADDR, &slaveConfig);
    I2C_SlaveTransferCreateHandle(I2C_BUS_BASEADDR, &slaveHandle, i2cSlaveCallback, NULL);
    slaveHandle.eventMask |= kI2C_SlaveCompletionEvent;
    I2C_SlaveTransferNonBlocking(I2C_BUS_BASEADDR, &slaveHandle, kI2C_SlaveCompletionEvent);
}

void InitLedDriver(void) {
    CLOCK_EnableClock(LED_DRIVER_SDB_CLOCK);
    PORT_SetPinMux(LED_DRIVER_SDB_PORT, LED_DRIVER_SDB_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(LED_DRIVER_SDB_GPIO, LED_DRIVER_SDB_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 0});
    GPIO_WritePinOutput(LED_DRIVER_SDB_GPIO, LED_DRIVER_SDB_PIN, 1);
}

void InitPeripherals(void)
{
    InitInterruptPriorities();
    InitLedDriver();
    InitTestLed();
    LedPwm_Init();
    InitI2c();
//    InitI2cWatchdog();
}

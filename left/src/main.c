#include "fsl_gpio.h"
#include "clock_config.h"
#include "fsl_port.h"
#include "key_matrix.h"

#define TEST_LED_GPIO  GPIOA
#define TEST_LED_PORT  PORTA
#define TEST_LED_CLOCK kCLOCK_PortA
#define TEST_LED_PIN   12

#define TEST_LED_INIT(output) GPIO_PinInit(TEST_LED_GPIO, TEST_LED_PIN, \
                                          &(gpio_pin_config_t){kGPIO_DigitalOutput, (output)})
#define TEST_LED_ON() GPIO_ClearPinsOutput(TEST_LED_GPIO, 1U << TEST_LED_PIN)
#define TEST_LED_OFF() GPIO_SetPinsOutput(TEST_LED_GPIO, 1U << TEST_LED_PIN)

#define KEYBOARD_MATRIX_COLS_NUM 7
#define KEYBOARD_MATRIX_ROWS_NUM 5

key_matrix_t keyMatrix = {
    .colNum = KEYBOARD_MATRIX_COLS_NUM,
    .rowNum = KEYBOARD_MATRIX_ROWS_NUM,
    .cols = (key_matrix_pin_t[]){
        {PORTB, GPIOB, kCLOCK_PortB, 11},
        {PORTA, GPIOA, kCLOCK_PortA, 6},
        {PORTA, GPIOA, kCLOCK_PortA, 8},
        {PORTB, GPIOB, kCLOCK_PortB, 0},
        {PORTB, GPIOB, kCLOCK_PortB, 6},
        {PORTA, GPIOA, kCLOCK_PortA, 3},
        {PORTB, GPIOB, kCLOCK_PortB, 5}
    },
    .rows = (key_matrix_pin_t[]){
        {PORTB, GPIOB, kCLOCK_PortB, 7},
        {PORTB, GPIOB, kCLOCK_PortB, 10},
        {PORTA, GPIOA, kCLOCK_PortA, 5},
        {PORTA, GPIOA, kCLOCK_PortA, 7},
        {PORTA, GPIOA, kCLOCK_PortA, 4}
    }
};

int main(void)
{
    CLOCK_EnableClock(TEST_LED_CLOCK);
    PORT_SetPinMux(TEST_LED_PORT, TEST_LED_PIN, kPORT_MuxAsGpio);
    TEST_LED_INIT(1);
    BOARD_BootClockRUN();

    KeyMatrix_Init(&keyMatrix);
    KeyMatrix_Scan(&keyMatrix);

    while (1)
    {
        for (uint32_t i=0; i<500000; i++) {
            TEST_LED_ON();
        }
        for (uint32_t i=0; i<500000; i++) {
            TEST_LED_OFF();
        }
    }
}

/*
#define I2C_DATA_LENGTH 2

#include "board.h"
#include "fsl_clock_manager.h"
#include "fsl_i2c_slave_driver.h"
#include "fsl_i2c_shared_function.h"
#include "i2c_addresses.h"
#include "main.h"

uint8_t isSw2Pressed;
uint8_t isSw3Pressed;
uint8_t buffer[I2C_DATA_LENGTH];

void I2C0_IRQHandler(void)
{
    I2C_DRV_IRQHandler(I2C0_IDX);
}

static void i2c_slave_callback(uint8_t instance, i2c_slave_event_t i2cEvent, void *param)
{
    i2c_slave_state_t *slaveState = I2C_DRV_SlaveGetHandler(instance);

    switch (i2cEvent) {
        case kI2CSlaveTxReq:
            slaveState->txSize = I2C_DATA_LENGTH;
            slaveState ->txBuff = buffer;
            slaveState ->isTxBusy = true;
            buffer[0] = isSw2Pressed;
            buffer[1] = isSw3Pressed;
            break;

        case kI2CSlaveTxEmpty:
            slaveState->isTxBusy = false;
            break;

        default:
            break;
    }
}

int main(void)
{
    // Initialize GPIO.

    gpio_input_pin_user_config_t inputPin[] =
    {
        {
            .pinName                       = kGpioSW2,
            .config.isPullEnable           = true,
            .config.pullSelect             = kPortPullUp,
            .config.isPassiveFilterEnabled = false,
            .config.interrupt              = kPortIntDisabled,
        },
        {
            .pinName                       = kGpioSW3,
            .config.isPullEnable           = true,
            .config.pullSelect             = kPortPullUp,
            .config.isPassiveFilterEnabled = false,
            .config.interrupt              = kPortIntDisabled,
        },
        {
            .pinName = GPIO_PINS_OUT_OF_RANGE,
        }
    };

    gpio_output_pin_user_config_t outputPin[] =
    {
        {
            .pinName              = kGpioLED1,
            .config.outputLogic   = 0,
            .config.slewRate      = kPortFastSlewRate,
            .config.driveStrength = kPortHighDriveStrength,
        },
        {
            .pinName              = kGpioLED3,
            .config.outputLogic   = 0,
            .config.slewRate      = kPortFastSlewRate,
            .config.driveStrength = kPortHighDriveStrength,
        },
        {
            .pinName = GPIO_PINS_OUT_OF_RANGE,
        }
    };

    GPIO_DRV_Init(inputPin, outputPin);

    // Initialize I2C.

    i2c_slave_state_t slave;

    i2c_slave_user_config_t userConfig = {
        .address         = I2C_ADDRESS_LED_DRIVER_LEFT,
        .slaveCallback   = i2c_slave_callback,
        .callbackParam   = NULL,
        .slaveListening  = true,
        .startStopDetect = true,
    };

    configure_i2c_pins(0);
    I2C_DRV_SlaveInit(BOARD_I2C_INSTANCE, &userConfig, &slave);

    // Update switch states and toggle LEDs accordingly.

    while (1) {
        isSw2Pressed = GPIO_DRV_ReadPinInput(kGpioSW2);
        isSw3Pressed = GPIO_DRV_ReadPinInput(kGpioSW3);
        GPIO_DRV_WritePinOutput(kGpioLED1, isSw2Pressed);
        GPIO_DRV_WritePinOutput(kGpioLED3, isSw3Pressed);
    }
}
*/

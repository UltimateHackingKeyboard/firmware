#include "fsl_gpio.h"
#include "init_clock.h"
#include "fsl_port.h"
#include "fsl_i2c.h"
#include "key_matrix.h"
#include "test_led.h"
#include "i2c_addresses.h"
#include "i2c.h"
#include "init_peripherials.h"

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

i2c_slave_config_t slaveConfig;
i2c_slave_handle_t slaveHandle;

static void i2c_slave_callback(I2C_Type *base, i2c_slave_transfer_t *xfer, void *userData)
{
    switch (xfer->event)
    {
        case kI2C_SlaveTransmitEvent:
            xfer->data = keyMatrix.keyStates;
            xfer->dataSize = KEYBOARD_MATRIX_COLS_NUM*KEYBOARD_MATRIX_ROWS_NUM;
            break;
        case kI2C_SlaveReceiveEvent:
            break;
        case kI2C_SlaveCompletionEvent:
            break;
        case kI2C_SlaveTransmitAckEvent:
            break;
        default:
            break;
    }
}

int main(void)
{
    InitClock();
    InitPeripherials();

    I2C_SlaveGetDefaultConfig(&slaveConfig);
    slaveConfig.slaveAddress = I2C_ADDRESS_LEFT_KEYBOARD_HALF;
    slaveConfig.addressingMode = kI2C_Address7bit/kI2C_RangeMatch;
    I2C_SlaveInit(I2C_BUS_BASEADDR, &slaveConfig);
    I2C_SlaveTransferCreateHandle(I2C_BUS_BASEADDR, &slaveHandle, i2c_slave_callback, NULL);
    I2C_SlaveTransferNonBlocking(I2C_BUS_BASEADDR, &slaveHandle, kI2C_SlaveCompletionEvent);

    KeyMatrix_Init(&keyMatrix);
    while (1) {
        KeyMatrix_Scan(&keyMatrix);
    }
}

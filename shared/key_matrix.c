#include "fsl_gpio.h"
#include "key_matrix.h"

void KeyMatrix_Init(key_matrix_t *keyMatrix)
{
    for (key_matrix_pin_t *row = keyMatrix->rows; row < keyMatrix->rows + keyMatrix->rowNum; row++) {
        CLOCK_EnableClock(row->clock);
        PORT_SetPinConfig(row->port, row->pin,
                          &(port_pin_config_t){.pullSelect=kPORT_PullDisable, .mux=kPORT_MuxAsGpio});
        GPIO_PinInit(row->gpio, row->pin, &(gpio_pin_config_t){kGPIO_DigitalOutput, 0});
    }

    for (key_matrix_pin_t *col = keyMatrix->cols; col < keyMatrix->cols + keyMatrix->colNum; col++) {
        CLOCK_EnableClock(col->clock);
        PORT_SetPinConfig(col->port, col->pin,
                          &(port_pin_config_t){.pullSelect=kPORT_PullDown, .mux=kPORT_MuxAsGpio});
        GPIO_PinInit(col->gpio, col->pin, &(gpio_pin_config_t){kGPIO_DigitalInput});
    }
}

void KeyMatrix_Scan(key_matrix_t *keyMatrix)
{
    uint8_t *keyState = keyMatrix->keyStates;

    key_matrix_pin_t *rowEnd = keyMatrix->rows + keyMatrix->rowNum;
    for (key_matrix_pin_t *row = keyMatrix->rows; row<rowEnd; row++) {
        GPIO_Type *rowGpio = row->gpio;
        uint32_t rowPin = row->pin;
        GPIO_WritePinOutput(rowGpio, rowPin, 1);

        key_matrix_pin_t *colEnd = keyMatrix->cols + keyMatrix->colNum;
        for (key_matrix_pin_t *col = keyMatrix->cols; col<colEnd; col++) {
            *(keyState++) = GPIO_ReadPinInput(col->gpio, col->pin);
        }

        GPIO_WritePinOutput(rowGpio, rowPin, 0);
        for (volatile uint32_t i=0; i<100; i++); // Wait for the new port state to settle. This avoid bogus key state detection.
    }
}

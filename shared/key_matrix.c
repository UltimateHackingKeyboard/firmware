#include "fsl_gpio.h"
#include "key_matrix.h"

uint8_t DebounceTimePress = 50, DebounceTimeRelease = 50;

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

void KeyMatrix_ScanRow(key_matrix_t *keyMatrix)
{
    uint8_t *keyState = keyMatrix->keyStates + keyMatrix->currentRowNum * keyMatrix->colNum;
    key_matrix_pin_t *row = keyMatrix->rows + keyMatrix->currentRowNum;

    key_matrix_pin_t *colEnd = keyMatrix->cols + keyMatrix->colNum;
    for (key_matrix_pin_t *col = keyMatrix->cols; col<colEnd; col++) {
        *(keyState++) = GPIO_ReadPinInput(col->gpio, col->pin);
    }

    GPIO_WritePinOutput(row->gpio, row->pin, 0);

    if (++keyMatrix->currentRowNum >= keyMatrix->rowNum) {
        keyMatrix->currentRowNum = 0;
    }

    // This should come last to maintain the strobe for as long as possible to minimize the chance of chatter.
    row = keyMatrix->rows + keyMatrix->currentRowNum;
    GPIO_WritePinOutput(row->gpio, row->pin, 1);
}

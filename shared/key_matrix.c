#include "fsl_gpio.h"
#include "key_matrix.h"

void KeyMatrix_Init(key_matrix_t *keyMatrix)
{
    for (uint8_t col_i=0; col_i<keyMatrix->colNum; col_i++) {
        key_matrix_pin_t col = keyMatrix->cols[col_i];
        CLOCK_EnableClock(col.clock);
        PORT_SetPinConfig(col.port, col.pin,
                          &(port_pin_config_t){.pullSelect=kPORT_PullDisable, .mux=kPORT_MuxAsGpio});
        GPIO_PinInit(col.gpio, col.pin, &(gpio_pin_config_t){kGPIO_DigitalOutput, 0});
    }

    for (uint8_t row_i=0; row_i<keyMatrix->rowNum; row_i++) {
        key_matrix_pin_t row = keyMatrix->rows[row_i];
        CLOCK_EnableClock(row.clock);
        PORT_SetPinConfig(row.port, row.pin,
                          &(port_pin_config_t){.pullSelect=kPORT_PullDown, .mux=kPORT_MuxAsGpio});
        GPIO_PinInit(row.gpio, row.pin, &(gpio_pin_config_t){kGPIO_DigitalInput});
    }
}

void KeyMatrix_Scan(key_matrix_t *keyMatrix)
{
    uint8_t keyId = 0;
    for (uint8_t col=0; col<keyMatrix->colNum; col++) {
        GPIO_WritePinOutput(keyMatrix->cols[col].gpio, keyMatrix->cols[col].pin, 1);
        for (uint8_t row=0; row<keyMatrix->rowNum; row++) {
            keyMatrix->keyStates[keyId++] = GPIO_ReadPinInput(keyMatrix->rows[row].gpio, keyMatrix->rows[row].pin);
        }
        GPIO_WritePinOutput(keyMatrix->cols[col].gpio, keyMatrix->cols[col].pin, 0);
    }
}

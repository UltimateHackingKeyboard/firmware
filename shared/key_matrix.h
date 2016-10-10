#ifndef __KEY_MATRIX_H__
#define __KEY_MATRIX_H__

// Includes:

    #include "fsl_common.h"
    #include "fsl_port.h"

// Type definitions:

    typedef struct {
        PORT_Type *port;
        GPIO_Type *gpio;
        clock_ip_name_t clock;
        uint32_t pin;
    } key_matrix_pin_t;

    typedef struct {
        uint8_t colNum;
        uint8_t rowNum;
        key_matrix_pin_t *cols;
        key_matrix_pin_t *rows;
    } key_matrix_t;

// Functions:

    void KeyMatrix_Init(key_matrix_t keyMatrix);

#endif

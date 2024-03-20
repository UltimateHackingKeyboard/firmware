#ifndef __KEY_MATRIX_H__
#define __KEY_MATRIX_H__

// Includes:

    #include <stdint.h>
#ifndef __ZEPHYR__
    #include "fsl_common.h"
    #include "fsl_port.h"

// Macros:

    #define MAX_KEYS_IN_MATRIX 100

// Typedefs:

    typedef struct {
        PORT_Type *port;
        GPIO_Type *gpio;
        clock_ip_name_t clock;
        uint32_t pin;
    } key_matrix_pin_t;

    typedef struct {
        uint8_t colNum;
        uint8_t rowNum;
        uint8_t currentRowNum;
        key_matrix_pin_t *cols;
        key_matrix_pin_t *rows;
        uint8_t keyStates[MAX_KEYS_IN_MATRIX];
    } key_matrix_t;
#endif

// Variables:

    extern uint8_t DebounceTimePress, DebounceTimeRelease;

// Functions:

#ifndef __ZEPHYR__
    void KeyMatrix_Init(key_matrix_t *keyMatrix);
    void KeyMatrix_ScanRow(key_matrix_t *keyMatrix);
#endif

#endif

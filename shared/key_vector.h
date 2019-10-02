#ifndef __KEY_VECTOR_H__
#define __KEY_VECTOR_H__

// Includes:

    #include "fsl_common.h"
    #include "fsl_port.h"

// Macros:

    #define MAX_KEYS_IN_VECTOR 20

// Typedefs:

    typedef struct {
        PORT_Type *port;
        GPIO_Type *gpio;
        clock_ip_name_t clock;
        uint32_t pin;
    } key_vector_pin_t;

    typedef struct {
        uint8_t itemNum;
        key_vector_pin_t *items;
        uint8_t keyStates[MAX_KEYS_IN_VECTOR];
    } key_vector_t;

// Variables:

//    extern uint8_t DebounceTimePress, DebounceTimeRelease;

// Functions:

    void KeyVector_Init(key_vector_t *keyVector);
    void KeyVector_Scan(key_vector_t *keyVector);

#endif

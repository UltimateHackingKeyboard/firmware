#ifndef __MODULE_H__
#define __MODULE_H__

// Includes:

    #include "layer.h"
    #include "slot.h"

// Macros:

    #define MAX_KEY_COUNT_PER_MODULE     64

// Typedefs:

    typedef struct {
        uint8_t acceleration;
        uint8_t maxSpeed;
        uint8_t roles[LAYER_COUNT];
    } pointer_t;

#endif

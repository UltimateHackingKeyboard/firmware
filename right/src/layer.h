#ifndef __LAYER_H__
#define __LAYER_H__

    #include <stdint.h>

// Macros:

    #define LAYER_ID_BASE  0
    #define LAYER_ID_MOD   1
    #define LAYER_ID_FN    2
    #define LAYER_ID_MOUSE 3

    #define LAYER_COUNT 4

    extern uint8_t ActiveLayer;

    void Layer_MoveTo(uint8_t layer);
    void Layer_MoveToBase();

#endif

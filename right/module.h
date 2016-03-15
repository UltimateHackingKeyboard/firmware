#ifndef __MODULE_H__
#define __MODULE_H__

// Includes:

    #include "action.h"
    #include "layer.h"
    #include "slot.h"

// Macros:

    #define MODULE_ID_LEFT_KEYBOARD_HALF 0
    #define MODULE_ID_KEY_CLUSTER_LEFT   1
    #define MODULE_ID_TRACKBALL_RIGHT    2
    #define MODULE_ID_TRACKPOINT_RIGHT   3
    #define MODULE_ID_TOUCHPAD           4

    #define MODULE_REQUEST_GET_PROTOCOL_VERSION 0
    #define MODULE_REQUEST_GET_MODULE_ID        1
    #define MODULE_REQUEST_GET_FEATURES         2
    #define MODULE_REQUEST_GET_STATE            3

    #define MAX_KEY_COUNT_PER_MODULE 64

// Typedefs:

    typedef struct {
        uint8_t moduleId;
        uint8_t pointerCount;
        uint8_t keyCount;
        uint8_t keyStates[MAX_KEY_COUNT_PER_MODULE];
        key_action_t keyActions[LAYER_COUNT][MAX_KEY_COUNT_PER_MODULE];
    } module_t;

// Variables:

    extern module_t AttachedModules[SLOT_COUNT];

#endif

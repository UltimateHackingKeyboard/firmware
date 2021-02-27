#ifndef __MODULE_H__
#define __MODULE_H__

// Includes:

    #include "layer.h"
    #include "slot.h"
    #include "slave_protocol.h"

// Macros:

    #define MAX_KEY_COUNT_PER_MODULE     64

// Typedefs:

    typedef struct {
        uint8_t acceleration;
        uint8_t maxSpeed;
        uint8_t roles[LAYER_COUNT];
    } pointer_t;

    typedef enum {
        NavigationMode_Cursor,
        NavigationMode_Scroll,
        NavigationMode_Caret,
        NavigationMode_Media,
    } navigation_mode_t;

// Variables:

    extern navigation_mode_t ModuleNavigationModes[ModuleId_Count][LayerId_Count];

#endif

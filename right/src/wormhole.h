#ifndef __WORMHOLE_RUNTIME_H__
#define __WORMHOLE_RUNTIME_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>
    #include "power_mode.h"


// Macros:

// Typedefs:

    typedef struct {
        power_mode_t restartPowerMode;
    } wormhole_data_t;

// Variables:

    extern wormhole_data_t StateWormhole;

// Functions:

#endif

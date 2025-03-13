#ifndef __WORMHOLE_RUNTIME_H__
#define __WORMHOLE_RUNTIME_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>
    #include "power_mode.h"


// Macros:

    #define WORMHOLE_MAGIC_NUMBER 0x3b04cd9e94521f9a
    #define IS_STATE_WORMHOLE_OPEN (StateWormhole.magicNumber == WORMHOLE_MAGIC_NUMBER)

// Typedefs:

    typedef struct {
        uint64_t magicNumber;
        power_mode_t restartPowerMode;
    } wormhole_data_t;

// Variables:

    extern wormhole_data_t StateWormhole;

// Functions:

#endif

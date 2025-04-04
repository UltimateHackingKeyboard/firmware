#ifndef __WORMHOLE_RUNTIME_H__
#define __WORMHOLE_RUNTIME_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>
    #include "power_mode.h"
    #include "macros/status_buffer.h"

// Macros:

    #define WORMHOLE_MAGIC_NUMBER 0x3b04cd9e94521f9a
    #define IS_STATE_WORMHOLE_OPEN (StateWormhole.magicNumber == WORMHOLE_MAGIC_NUMBER)

// Typedefs:

    typedef struct {
        uint64_t magicNumber;

        bool rebootToPowerMode;
        power_mode_t restartPowerMode;

        bool persistStatusBuffer;
        macro_status_buffer_t statusBuffer;
    } wormhole_data_t;

    void StateWormhole_Close();

// Variables:

    extern wormhole_data_t StateWormhole;

// Functions:



#endif

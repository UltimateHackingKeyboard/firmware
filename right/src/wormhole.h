#ifndef __WORMHOLE_RUNTIME_H__
#define __WORMHOLE_RUNTIME_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>
    #include "power_mode.h"
    #include "macros/status_buffer.h"
    #include "trace.h"

// Macros:

// Typedefs:

    typedef struct {
        uint64_t magicNumber;

        bool rebootToPowerMode;
        power_mode_t restartPowerMode;

        bool persistStatusBuffer;
        macro_status_buffer_t statusBuffer;

        trace_buffer_t traceBuffer;

    } wormhole_data_t;

    bool StateWormhole_IsOpen(void);
    void StateWormhole_Open(void);
    void StateWormhole_Close(void);
    void StateWormhole_Clean(void);

// Variables:

    extern wormhole_data_t StateWormhole;

// Functions:



#endif

#ifndef __TRACE_DEFS_H__
#define __TRACE_DEFS_H__

// Includes:

    #include <inttypes.h>
    #include "debug.h"
    #include "logger.h"

// Macros:

    #define TRACE_BUFFER_SIZE 256

// Typedefs:

    typedef struct {
        char data[TRACE_BUFFER_SIZE];
        uint32_t eventVector;
        uint16_t position;
    } ATTR_PACKED trace_buffer_t;

    void Trace_Print(log_target_t additionalLogTargets, const char* reason);

#endif

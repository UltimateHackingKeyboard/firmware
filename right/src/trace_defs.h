#ifndef __TRACE_DEFS_H__
#define __TRACE_DEFS_H__

// Includes:

    #include <inttypes.h>
    #include "debug.h"

// Macros:

    #define TRACE_BUFFER_SIZE 256

// Typedefs:

    typedef struct {
        char data[TRACE_BUFFER_SIZE];
        uint32_t eventVector;
        uint16_t position;
    } ATTR_PACKED trace_buffer_t;

#endif

#ifndef __TRACE_H__
#define __TRACE_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "debug.h"

// Macros:

#define TRACE_BUFFER_SIZE 128

// Typedefs:

    typedef struct {
        char data[TRACE_BUFFER_SIZE];
        uint32_t eventVector;
        uint16_t position;
    } ATTR_PACKED trace_buffer_t;

// Variables:

// Functions:

    void Trace_Init(void);
    void Trace(char a);
    void Trace_Print(void);
    void Trace_Printf(const char *fmt, ...);

#endif


#ifndef __TRACE_H__
#define __TRACE_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "debug.h"

// Macros:

// Typedefs:

// Variables:

// Functions:

    void Trace_Init(void);
    void Trace(char a);
    void Trace_Print(void);
    void Trace_Printf(const char *fmt, ...);

#endif


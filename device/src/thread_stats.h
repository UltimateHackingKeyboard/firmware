#ifndef __THREAD_STATS_H__
#define __THREAD_STATS_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "debug.h"

// Macros:

// Typedefs:

// Variables:

// Functions:

#if DEBUG_THREAD_STATS
    void ThreadStats_Init(void);
    void ThreadStats_Switch(void);
    void ThreadStats_Print(void);
    void ThreadStats_Snap(void);
#endif

#endif


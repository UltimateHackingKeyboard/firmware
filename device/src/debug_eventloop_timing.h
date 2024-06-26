#ifndef __DEBUG_EVENTLOOP_TIMING_H__
#define __DEBUG_EVENTLOOP_TIMING_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "legacy/debug.h"

// Macros:

// Typedefs:

// Variables:

// Functions:

#if DEBUG_EVENTLOOP_TIMING
    void EventloopTiming_Start();
    void EventloopTiming_Switch();
    void EventloopTiming_End();
    void EventloopTiming_Watch(const char* section);
    void EventloopTiming_WatchReset();
#endif

#endif

#ifndef __CALENDAR_H__
#define __CALENDAR_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "timer.h"

// Macros:

// Typedefs:

    typedef enum {
        CalendarEvent_SegmentDisplayUpdate,
        CalendarEvent_MacroWakeOnTime,
        CalendarEvent_Count
    } calendar_event_t;

// Variables:

    extern bool Calendar_IsActive;

// Functions:

    void Calendar_Schedule(uint32_t at, calendar_event_t evt);
    void Calendar_Reschedule(uint32_t at, calendar_event_t evt);
    void Calendar_Process();

#endif

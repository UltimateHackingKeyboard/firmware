#ifndef __EVENT_SCHEDULER_H__
#define __EVENT_SCHEDULER_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "timer.h"
    #include "debug.h"

// Macros:

// Typedefs:

    typedef enum {
        EventSchedulerEvent_SegmentDisplayUpdate,
        EventSchedulerEvent_MacroWakeOnTime,
        EventSchedulerEvent_MacroRecorderFlashing,
        EventSchedulerEvent_SwitchScreen,
        EventSchedulerEvent_ShiftScreen,
        EventSchedulerEvent_UpdateBattery,
        EventSchedulerEvent_Postponer,
        EventSchedulerEvent_NativeActions,
        EventSchedulerEvent_AgentLed,
        EventSchedulerEvent_UpdateLedSleepModes,
        EventSchedulerEvent_MouseController,
        EventSchedulerEvent_ReenableUart,
        EventSchedulerEvent_Count
    } event_scheduler_event_t;

    typedef enum {
       // events that trigger event loop execution

       // main code units
       EventVector_MacroEngine =                           1 << 0,
       EventVector_StateMatrix =                           1 << 1,
       EventVector_NativeActions =                         1 << 2,
       EventVector_MouseKeys =                             1 << 3,
       EventVector_MouseController =                       1 << 4,
       EventVector_Postponer =                             1 << 5,
       EventVector_LayerHolds =                            1 << 6,
       EventVector_EventScheduler =                        1 << 7,

       // some other minor triggers
       EventVector_KeyboardLedState =                      1 << 8,
       EventVector_UsbMacroCommandWaitingForExecution =    1 << 9,
       EventVector_ProtocolChanged =                       1 << 10,
       EventVector_LedManagerFullUpdateNeeded =            1 << 11,
       EventVector_KeymapReloadNeeded =                    1 << 12,
       EventVector_SegmentDisplayNeedsUpdate =             1 << 13,
       EventVector_LedMapUpdateNeeded =                    1 << 14,

       EventVector_ReportUpdateMask = ((1 << 8) - 1) & ~EventVector_EventScheduler,
       EventVector_UserLogicUpdateMask = ((1 << 15) - 1) & ~EventVector_EventScheduler,

       // events that are informational only
       EventVector_NativeActionReportsUsed =               1 << 16,
       EventVector_MacroReportsUsed =                      1 << 17,
       EventVector_MouseKeysReportsUsed =                  1 << 18,
       EventVector_MouseControllerMouseReportsUsed =       1 << 19,
       EventVector_MouseControllerKeyboardReportsUsed =    1 << 20,
       EventVector_ReportsChanged =                        1 << 21,
       EventVector_NativeActionsPostponing =               1 << 22,
       EventVector_MacroEnginePostponing =                 1 << 23,
    } event_vector_event_t;

// Variables:

    extern uint32_t EventScheduler_Vector;

// Functions:

    void EventScheduler_Reschedule(uint32_t at, event_scheduler_event_t evt, const char* label);
    void EventScheduler_Schedule(uint32_t at, event_scheduler_event_t evt, const char* label);
    void EventScheduler_Unschedule(event_scheduler_event_t evt);
    uint32_t EventScheduler_Process();

    void EventVector_ReportMask(const char* prefix, uint32_t mask);

    static inline void EventVector_Set(event_vector_event_t evt) {
        EventScheduler_Vector |= evt;
        LOG_SCHEDULE(
            if (evt != EventVector_EventScheduler) {
                EventVector_ReportMask("    +++ ", evt);
            }
        );
    };

    static inline bool EventVector_IsSet(event_vector_event_t evt) {
        bool res = EventScheduler_Vector & evt;
        LOG_SCHEDULE(
            if (res) {
                EventVector_ReportMask("        ??? ", evt);
            }
        );
        return res;
    };

    static inline void EventVector_Unset(event_vector_event_t evt) {
        LOG_SCHEDULE(
            if (EventScheduler_Vector & evt) {
                EventVector_ReportMask("        --- ", evt);
            }
        );
        EventScheduler_Vector &= ~evt;
    };

    static inline void EventVector_SetValue(event_vector_event_t evt, bool active) {
        if (active) {
            EventVector_Set(evt);
        } else {
            EventVector_Unset(evt);
        }
    };

#endif

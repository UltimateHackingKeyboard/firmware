#ifndef __EVENT_SCHEDULER_H__
#define __EVENT_SCHEDULER_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "timer.h"
    #include "debug.h"
    #include "atomicity.h"

#ifdef __ZEPHYR__
    #include "main.h"
#endif

// Macros:

// Typedefs:

    typedef enum {
        EventSchedulerEvent_ModuleConnectionStatusUpdate,
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
        EventSchedulerEvent_UpdateMergeSensor,
        EventSchedulerEvent_PowerMode,
        EventSchedulerEvent_EndBtPairing,
        EventSchedulerEvent_RestartBt,
        EventSchedulerEvent_BtStartAdvertisement,
        EventSchedulerEvent_BtStartScanning,
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
       EventVector_SendUsbReports =                        1 << 7,
       EventVector_ResendUsbReports =                      1 << 8,
       EventVector_EventScheduler =                        1 << 9,
       EventVector_MainTriggers =                          ((EventVector_EventScheduler << 1) - 1),


       // some other minor triggers
       EventVector_KeyboardLedState =                      1 << 10,
       EventVector_UsbMacroCommandWaitingForExecution =    1 << 11,
       EventVector_ProtocolChanged =                       1 << 12,
       EventVector_LedManagerFullUpdateNeeded =            1 << 13,
       EventVector_KeymapReloadNeeded =                    1 << 14,
       EventVector_SegmentDisplayNeedsUpdate =             1 << 15,
       EventVector_LedMapUpdateNeeded =                    1 << 16,
       EventVector_ApplyConfig =                           1 << 17,
       EventVector_NewMessage =                            1 << 18,
       EventVector_AuxiliaryTriggers =                     ((EventVector_NewMessage << 1) - 1),

       // events that are informational only
       EventVector_NativeActionReportsUsed =               1 << 19,
       EventVector_MacroReportsUsed =                      1 << 20,
       EventVector_MouseKeysReportsUsed =                  1 << 21,
       EventVector_MouseControllerMouseReportsUsed =       1 << 22,
       EventVector_MouseControllerKeyboardReportsUsed =    1 << 23,
       EventVector_NativeActionsPostponing =               1 << 24,
       EventVector_KeystrokeDelayPostponing =              1 << 25,
       EventVector_MacroEnginePostponing =                 1 << 26,
       EventVector_MouseControllerPostponing =             1 << 27,

       // helper masks
       EventVector_ReportUpdateMask = EventVector_MainTriggers & ~EventVector_EventScheduler,
       EventVector_UserLogicUpdateMask = EventVector_AuxiliaryTriggers & ~EventVector_EventScheduler,
       EventVector_SomeonePostponing = EventVector_KeystrokeDelayPostponing | EventVector_NativeActionsPostponing | EventVector_MacroEnginePostponing | EventVector_MouseControllerPostponing,
    } event_vector_event_t;

/**
 * Notes on EventVector flags:
 *
 * EventVector_Postponer means that Postponer is active. There may be events in the postponer queue while someone is postponing while sleeping (e.g., macro engine waiting for future events.), and in this case EventVector_Postponer is unset.
 * */

// Variables:

    extern volatile uint32_t EventScheduler_Vector;

// Functions:

    void EventScheduler_Reschedule(uint32_t at, event_scheduler_event_t evt, const char* label);
    void EventScheduler_Schedule(uint32_t at, event_scheduler_event_t evt, const char* label);
    void EventScheduler_Unschedule(event_scheduler_event_t evt);
    uint32_t EventScheduler_Process();

    void EventVector_ReportMask(const char* prefix, uint32_t mask);

    static inline void EventVector_Set(event_vector_event_t evt) {
        DISABLE_IRQ();
        EventScheduler_Vector |= evt;
        ENABLE_IRQ();
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
        DISABLE_IRQ();
        EventScheduler_Vector &= ~evt;
        ENABLE_IRQ();
    };

    static inline void EventVector_SetValue(event_vector_event_t evt, bool active) {
        if (active) {
            EventVector_Set(evt);
        } else {
            EventVector_Unset(evt);
        }
    };

    static inline void EventVector_WakeMain() {
#ifdef __ZEPHYR__
        Main_Wake();
#endif
    }

#endif

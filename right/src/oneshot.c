#include "oneshot.h"
#include "macros/core.h"
#include "timer.h"
#include "config_manager.h"
#include "event_scheduler.h"


oneshot_state_t OneShot_State = OneShotState_Idle;
static bool oneShot_LastKeyWasOneshot = false;
static uint32_t oneshotActivationTime = 0;


void OneShot_Activate(uint32_t atTime) {
    OneShot_State = OneShotState_WoundUp;
    oneShot_LastKeyWasOneshot = true;
    oneshotActivationTime = atTime;
    if (Cfg.Macros_OneShotTimeout != 0) {
        EventScheduler_Reschedule(oneshotActivationTime + Cfg.Macros_OneShotTimeout, EventSchedulerEvent_OneShotTimeout, "OneShot timeout");
    }
}

void OneShot_SignalInterrupt() {
    oneShot_LastKeyWasOneshot = false;
    EventVector_Set(EventVector_NativeActions);
}

void OneShot_OnTimeout() {
    if (!OneShotState_WoundUp) {
        return;
    }

    if (Timer_GetCurrentTime() - oneshotActivationTime < Cfg.Macros_OneShotTimeout) {
        EventScheduler_Schedule(oneshotActivationTime + Cfg.Macros_OneShotTimeout, EventSchedulerEvent_OneShotTimeout, "OneShot spurious wake");
        return;
    }

    OneShot_State = OneShotState_Unwinding;
    oneShot_LastKeyWasOneshot = false;
    EventVector_Set(EventVector_NativeActions);
}

bool OneShot_ShouldPostpone(bool queueNonEmpty, bool someonePostponing) {
    if (OneShot_State == OneShotState_WoundUp && !oneShot_LastKeyWasOneshot) {
        // If no one is postponing it means that it has finished its work.
        // Deactivate oneshot and wait until all macros in the queue deactivate too.
        if (!someonePostponing) {
            OneShot_State = OneShotState_Unwinding;
            Macros_WakeBecauseOfOneShot();
            return true;
        }
    }

    if (OneShot_State == OneShotState_Unwinding) {
        // keep postponing until all macros in the queue deactivate too.
        if (Macros_WakeMeOnOneShot) {
            Macros_WakeBecauseOfOneShot();
            return true;
        } else {
            OneShot_State = OneShotState_Idle;
            return false;
        }
    }

    return false;
}



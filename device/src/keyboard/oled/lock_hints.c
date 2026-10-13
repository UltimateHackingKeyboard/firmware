#include "lock_hints.h"
#include "device.h"
#include "device_state.h"
#include "connections.h"
#include "event_scheduler.h"
#include "power_mode.h"
#include "timer.h"
#include "screens/notification_screen.h"

#if DEVICE_HAS_OLED

#define WAKE_HINT_DURATION 3000

// Left half takes a while to reconnect, so hint only after a grace period.
#define LEFT_HALF_GRACE_PERIOD 5000
#define LEFT_HALF_HINT_DURATION 1000
#define LEFT_HALF_HINT_REFRESH_PERIOD 2000

bool LockHints_WakeHintShown = false;

static bool isAwake() {
    return CurrentPowerMode <= PowerMode_LastAwake;
}

// Shown when a wrong key is pressed while we are locked or asleep.
void LockHints_ShowWakeHint(void) {
    LockHints_WakeHintShown = true;
    NotificationScreen_NotifyFor("Press J+L", 0);
    EventScheduler_Reschedule(Timer_GetCurrentTime() + WAKE_HINT_DURATION, EventSchedulerEvent_LockHintsHideWakeHint, "hide wake hint");
}

void LockHints_HideWakeHint(void) {
    if (LockHints_WakeHintShown) {
        LockHints_WakeHintShown = false;
        EventScheduler_Unschedule(EventSchedulerEvent_LockHintsHideWakeHint);
        NotificationScreen_Hide();
    }
}

void LockHints_RestartLeftHalfGracePeriod(void) {
    EventScheduler_Reschedule(Timer_GetCurrentTime() + LEFT_HALF_GRACE_PERIOD, EventSchedulerEvent_LockHintsLeftHalf, "left half grace period");
}

// Blinks the hint for as long as we are awake and the left half is missing.
void LockHints_UpdateLeftHalfHint(void) {
    bool leftMissing = isAwake() && !DeviceState_IsTargetConnected(ConnectionTarget_Left);

    if (leftMissing) {
        NotificationScreen_NotifyFor("Press S+F / charge left", LEFT_HALF_HINT_DURATION);
        EventScheduler_Reschedule(Timer_GetCurrentTime() + LEFT_HALF_HINT_REFRESH_PERIOD, EventSchedulerEvent_LockHintsLeftHalf, "left half hint refresh");
    }
}

void LockHints_OnPowerModeChanged(void) {
    if (isAwake()) {
        LockHints_HideWakeHint();
        LockHints_RestartLeftHalfGracePeriod();
    } else if (!LockHints_WakeHintShown) {
        // Don't carry the left half hint over into sleep.
        NotificationScreen_Hide();
    }
}

#endif

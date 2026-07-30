#include "power_mode.h"
#include "attributes.h"
#include "timer.h"
#include "event_scheduler.h"
#include "led_manager.h"
#include "wormhole.h"
#include "stubs.h"
#include "hid/transport.h"
#include "usb_state.h"
#include "usb_report_updater.h"

#ifdef __ZEPHYR__
    #include "device_state.h"
    #include "connections.h"
    #include "bt_conn.h"
    #include "bt_manager.h"
    #include "keyboard/uart_bridge.h"
    #include "keyboard/key_scanner.h"
    #include "keyboard/charger.h"
    #include "keyboard/leds.h"
    #include "state_sync.h"
#else
    #include "slave_drivers/is31fl3xxx_driver.h"
#endif

power_mode_config_t PowerModeConfig[PowerMode_Count] = {
    [PowerMode_Awake] = {
        .name = "Awake",
        .i2cInterval = 1,
        .keyScanInterval = 1,
    },
    [PowerMode_Powersaving] = {
        .name = "Awake",
        .i2cInterval = 1,
        .keyScanInterval = 5,
    },
    [PowerMode_LightSleep] = {
        .name = "LightSleep",
        .i2cInterval = 1,
        .keyScanInterval = 10,
    },
    [PowerMode_Lock] = {
        .name = "Lock",
        .i2cInterval = 50,
        .keyScanInterval = 50,
    },
    [PowerMode_DeepSleep] = {
        .name = "DeepSleep",
        .i2cInterval = 100,
        .keyScanInterval = 100,
    },
};

static uint32_t lastWakeEvent = 0;

volatile power_mode_t CurrentPowerMode = PowerMode_Awake;

#define LIGHT_SLEEP_NOHOST_WAKEUP_LENGTH 5*1000


static bool isSomeoneAwake() {
#ifdef __ZEPHYR__
    bool someoneAwake = false;
    if (DEVICE_IS_UHK80_LEFT) {
        connection_target_t ourMaster = DEVICE_IS_UHK80_LEFT ? ConnectionTarget_Right : ConnectionTarget_Host;
        someoneAwake = DeviceState_IsTargetConnected(ourMaster);
    }
    if (DEVICE_IS_UHK80_RIGHT) {
        someoneAwake = Connections_IsCurrentHostAwake();
    }
#else
    bool someoneAwake = UsbState_Awake && UsbState_TransportUp;
#endif
    return someoneAwake;
}

void PowerMode_Update() {
    bool someoneAwake = isSomeoneAwake();

    power_mode_t newPowerMode = someoneAwake ? PowerMode_Awake : PowerMode_LightSleep;

    if (CurrentPowerMode <= PowerMode_LightSleep) {
        if (newPowerMode <= PowerMode_Powersaving) {
            PowerMode_ActivateMode(newPowerMode, false, false, "power mode update");
            return;
        }

        if (newPowerMode == PowerMode_LightSleep && CurrentPowerMode < PowerMode_LightSleep) {
            uint32_t baseTime = MAX(lastWakeEvent, UsbReportUpdater_LastActivityTime);
            if (Timer_GetCurrentTime() - baseTime >= LIGHT_SLEEP_NOHOST_WAKEUP_LENGTH) {
                PowerMode_ActivateMode(newPowerMode, false, false, "power mode update");
            } else {
                EventScheduler_Schedule(baseTime + LIGHT_SLEEP_NOHOST_WAKEUP_LENGTH, EventSchedulerEvent_PowerModeUpdate, "no host short wakeup");
            }
        }
    }
}

static void notifyEveryone() {
#ifdef __ZEPHYR__
    StateSync_UpdateProperty(StateSyncPropertyId_PowerMode, NULL);
#endif

    EventVector_Set(EventVector_LedManagerFullUpdateNeeded);
    EventVector_WakeMain();
}

static void lightSleep() {
    CurrentPowerMode = PowerMode_LightSleep;
    LedManager_RecalculateLedBrightness();
    notifyEveryone();
}

static void lock() {
    CurrentPowerMode = PowerMode_Lock;
    notifyEveryone();
}

static void deepSleep() {
    CurrentPowerMode = PowerMode_DeepSleep;
    notifyEveryone();
}

static void wake() {
    lastWakeEvent = Timer_GetCurrentTime();
    EventScheduler_Schedule(lastWakeEvent + LIGHT_SLEEP_NOHOST_WAKEUP_LENGTH, EventSchedulerEvent_PowerModeUpdate, "waked - check for short wake condition");
    CurrentPowerMode = PowerMode_Awake;
    notifyEveryone();

}

void PowerMode_ActivateMode(power_mode_t mode, bool toggle, bool force, const char* reason) {
    // if toggling a mode that's currently active, wake up
    if (CurrentPowerMode == mode && toggle) {
        mode = PowerMode_Awake;
    }

    // if two modes are activated, always sink into the deeper of them
    if (mode > PowerMode_LastAwake && CurrentPowerMode > mode && !force) {
        return;
    }

    // if we are already in the requested mode, do nothing
    if (CurrentPowerMode == mode) {
        return;
    }

    ATTR_UNUSED bool wasAwake = CurrentPowerMode <= PowerMode_LastAwake;
    ATTR_UNUSED bool wasDeepSleep = CurrentPowerMode == PowerMode_DeepSleep;

    switch (mode) {
        case PowerMode_Awake:
            wake();
            break;
        case PowerMode_LightSleep:
            lightSleep();
            break;
        case PowerMode_Lock:
            lock();
            break;
        case PowerMode_DeepSleep:
            deepSleep();
            break;
        default:
            break;
    }

#ifdef __ZEPHYR__
    LogU("Entered %s power mode\n", PowerModeConfig[CurrentPowerMode].name);
#else
    if (DEBUG_UHK60_SLEEPS) {
        Macros_Printf("Entered %s power mode, because: %s\n", PowerModeConfig[CurrentPowerMode].name, reason);
    }
#endif

#ifdef __ZEPHYR__
    // On an awake<->sleep transition, recompute BLE intervals (everything relaxes while asleep).
    if ((CurrentPowerMode <= PowerMode_LastAwake) != wasAwake) {
        BtConn_UpdateConnectionLatencies();
    }
#endif

#ifdef __ZEPHYR__
    // Leaving deep sleep has to bring the links back before anything wants to talk over
    // them. Entering it is deferred, so that the mode change itself still gets synced to
    // the other half over the link we are about to cut.
    if (CurrentPowerMode == PowerMode_DeepSleep) {
        EventScheduler_Schedule(Timer_GetCurrentTime() + POWER_MODE_DEEP_SLEEP_DELAY, EventSchedulerEvent_EnterDeepSleep, "cut links for deep sleep");
    } else {
        EventScheduler_Unschedule(EventSchedulerEvent_EnterDeepSleep);
        PowerMode_SetLinksEnabled(true);

        if (wasDeepSleep && DEVICE_IS_UHK80_LEFT) {
            // Each half wakes on its own sfjl combo. If the right one didn't, go back down
            // rather than idling awake on battery.
            EventScheduler_Schedule(Timer_GetCurrentTime() + 60*1000, EventSchedulerEvent_PutBackToShutDown, "we woke, but the right half may not have");
        }
    }
#endif
}

#ifdef __ZEPHYR__
// Deep sleep runs with the radio and the inter-half link down - together they are the bulk
// of what is still drawing power once the display and the backlight are off. Both halves
// reach this independently, and each wakes on its own sfjl combo.
void PowerMode_SetLinksEnabled(bool enabled) {
    static bool linksEnabled = true;

    if (linksEnabled == enabled) {
        return;
    }
    linksEnabled = enabled;

    if (enabled) {
#if DEVICE_IS_KEYBOARD
        UartBridge_Resume();
#endif
        BtManager_StartBt();
    } else {
        BtManager_StopBt();
#if DEVICE_IS_KEYBOARD
        UartBridge_Suspend();
#endif
    }
}

void PowerMode_EnterDeepSleep(void) {
    if (CurrentPowerMode == PowerMode_DeepSleep) {
        PowerMode_SetLinksEnabled(false);
    }
}
#endif

#if DEVICE_IS_KEYBOARD && defined(__ZEPHYR__)

void PowerMode_PutBackToSleepMaybe(void) {
    if (DEVICE_IS_UHK80_LEFT && CurrentPowerMode >= PowerMode_LightSleep && !DeviceState_IsDeviceConnected(DeviceId_Uhk80_Right)) {
        PowerMode_ActivateMode(PowerMode_DeepSleep, false, false, "put back to sleep because right side is not available");
    }
}

#endif

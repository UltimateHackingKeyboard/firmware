#include "power_mode.h"
#include "usb_composite_device.h"
#include "event_scheduler.h"
#include "led_manager.h"
#include "wormhole.h"
#include "stubs.h"

#ifdef __ZEPHYR__
    #include "device_state.h"
    #include "usb/usb.h"
    #include "connections.h"
    #include "keyboard/key_scanner.h"
    #include "keyboard/charger.h"
    #include "keyboard/leds.h"
    #include "state_sync.h"
    #include "proxy_log_backend.h"
#else
    #include "slave_drivers/is31fl3xxx_driver.h"
    #include "usb_composite_device.h"
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
    [PowerMode_SfjlSleep] = {
        .name = "SfjlSleep",
        .i2cInterval = 100,
        .keyScanInterval = 100,
    },
    [PowerMode_AutoShutDown] = {
        .name = "AutoShutDown",
        .i2cInterval = 500,
        .keyScanInterval = 500,
    },
    [PowerMode_ManualShutDown] = {
        .name = "ManualShutDown",
        .i2cInterval = 500,
        .keyScanInterval = 500,
    },
};

ATTR_UNUSED static bool usbAwake = false;

volatile power_mode_t CurrentPowerMode = PowerMode_Awake;

// originally written for Benedek's power callback
// TODO: remove this and simplify the rest of the code if the callback is not used.
void PowerMode_SetUsbAwake(bool awake) {
#if DEVICE_IS_UHK80_RIGHT
    usbAwake = awake;
    EventScheduler_Reschedule(Timer_GetCurrentTime() + POWER_MODE_UPDATE_DELAY, EventSchedulerEvent_PowerMode, "update sleep mode from power callback");
#endif
}

static bool isSomeoneAwake() {
#ifdef __ZEPHYR__
    connection_target_t ourMaster = DEVICE_IS_UHK80_LEFT ? ConnectionTarget_Right : ConnectionTarget_Host;
    bool someoneAwake = DeviceState_IsTargetConnected(ourMaster);
#else
    bool someoneAwake = CurrentPowerMode == PowerMode_Awake;
#endif
    return someoneAwake;
}

void PowerMode_Update() {
    bool someoneAwake = isSomeoneAwake();

    power_mode_t newPowerMode = someoneAwake ? PowerMode_Awake : PowerMode_LightSleep;

    if (newPowerMode == someoneAwake) {
        newPowerMode = RunningOnBattery ? PowerMode_Powersaving : PowerMode_Awake;
    }

    if (CurrentPowerMode <= PowerMode_LightSleep) {
        PowerMode_ActivateMode(newPowerMode, false, false, "power mode update");
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

static void sfjlSleep() {
    CurrentPowerMode = PowerMode_SfjlSleep;
    notifyEveryone();
}

static void shutDown(power_mode_t mode) {
    CurrentPowerMode = mode;
    notifyEveryone();
}

static void wake() {
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
        case PowerMode_SfjlSleep:
            sfjlSleep();
            break;
        case PowerMode_ManualShutDown:
        case PowerMode_AutoShutDown:
            shutDown(mode);
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

    if (CurrentPowerMode > PowerMode_Lock) {
        EventScheduler_Schedule(Timer_GetCurrentTime() + POWER_MODE_RESTART_DELAY, EventSchedulerEvent_PowerModeRestart, "restart power mode");
    }
}

void PowerMode_WakeHost() {
#ifdef __ZEPHYR__
    USB_RemoteWakeup();
#else
    WakeUpHost();
#endif
}

void PowerMode_Restart() {
#if DEVICE_IS_KEYBOARD && defined(__ZEPHYR__)
    StateWormhole_Open();
    StateWormhole.wasReboot = true;
    StateWormhole.rebootToPowerMode = true;
    StateWormhole.restartPowerMode = CurrentPowerMode;
    NVIC_SystemReset();
#endif
}

#if DEVICE_IS_KEYBOARD && defined(__ZEPHYR__)

power_mode_t lastDeepPowerMode = PowerMode_Awake;

static void runDepletedSleep(bool allowWake) {
    // if the keyboard is powered, give the user a chance to disconnect it for 30 seconds.
    while (!allowWake && !Charger_ShouldRemainInDepletedMode(false) && k_uptime_get() < 30*1000) {
        k_sleep(K_MSEC(PowerModeConfig[CurrentPowerMode].keyScanInterval));
        printk("In order to shut down the keyboard, please disconnect UHK from power.\n");
    }

    LogU("Entering low power mode. Allow wake by voltage raise %d\n", allowWake);

    while (Charger_ShouldRemainInDepletedMode(allowWake)) {
        k_sleep(K_MSEC(1000));
    }
    LogU("Waking from low power mode.");
}

static void runSfjlSleep() {
    while (true) {
        if (KeyScanner_ScanAndWakeOnSfjl(true, false)) {
            return;
        }

        if (Charger_ShouldEnterDepletedMode()) {
            runDepletedSleep(true);
        }

        k_sleep(K_MSEC(PowerModeConfig[CurrentPowerMode].keyScanInterval));
    }
}

void PowerMode_PutBackToSleepMaybe(void) {
    if (DEVICE_IS_UHK80_LEFT && CurrentPowerMode >= PowerMode_LightSleep && !DeviceState_IsDeviceConnected(DeviceId_Uhk80_Right)) {
        power_mode_t newMode = lastDeepPowerMode == PowerMode_ManualShutDown ? PowerMode_ManualShutDown : PowerMode_SfjlSleep;
        PowerMode_ActivateMode(newMode, false, false, "put back to sleep because right side is not available");
    }
}

void PowerMode_RestartedTo(power_mode_t mode) {
    CurrentPowerMode = mode;
    KeyBacklightBrightness = 0;
    lastDeepPowerMode = mode;

    InitProxyLogBackend();
    InitLeds_Min();
    InitKeyScanner_Min();
    InitCharger_Min();

    switch (mode) {
        case PowerMode_SfjlSleep:
            runSfjlSleep();
            break;
        case PowerMode_AutoShutDown:
        case PowerMode_ManualShutDown:
            printk("A mode %d\n", mode);
            runDepletedSleep(mode == PowerMode_AutoShutDown);
            break;
        default:
            break;
    }

    if (DEVICE_IS_UHK80_LEFT) {
        PowerMode_ActivateMode(PowerMode_LightSleep, false, true, "woken up from sfjl sleep, waiting for right");
        EventScheduler_Schedule(Timer_GetCurrentTime() + 60*1000, EventSchedulerEvent_PutBackToShutDown, "We were woken up, but right may not.");
    } else {
        PowerMode_ActivateMode(PowerMode_Awake, false, true, "woken up from sfjl sleep");
    }
}

#else

void PowerMode_RestartedTo(power_mode_t mode) {};

#endif

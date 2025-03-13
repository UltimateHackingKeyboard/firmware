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
    [PowerMode_ShutDown] = {
        .name = "ShutDown",
        .i2cInterval = 100,
        .keyScanInterval = 100,
    },
};

ATTR_UNUSED static bool usbAwake = false;

power_mode_t CurrentPowerMode = PowerMode_Awake;

// originally written for Benedek's power callback
// TODO: remove this and simplify the rest of the code if the callback is not used.
void PowerMode_SetUsbAwake(bool awake) {
#if DEVICE_IS_UHK80_RIGHT
    usbAwake = awake;

    CurrentTime = k_uptime_get();
    EventScheduler_Reschedule(CurrentTime + POWER_MODE_UPDATE_DELAY, EventSchedulerEvent_PowerMode, "update sleep mode from power callback");
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
        PowerMode_ActivateMode(newPowerMode, false);
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

static void shutDown() {
    CurrentPowerMode = PowerMode_ShutDown;
    notifyEveryone();
}

static void wake() {
    CurrentPowerMode = PowerMode_Awake;
    notifyEveryone();
}

void PowerMode_ActivateMode(power_mode_t mode, bool toggle) {
    // if toggling a mode that's currently active, wake up
    if (CurrentPowerMode == mode && toggle) {
        mode = PowerMode_Awake;
    }

    // if two modes are activated, always sink into the deeper of them
    if (mode > PowerMode_LastAwake && CurrentPowerMode > mode) {
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
        case PowerMode_ShutDown:
            shutDown();
            break;
        default:
            break;
    }

#ifdef __ZEPHYR__
    LogU("Entered %s power mode\n", PowerModeConfig[CurrentPowerMode].name);
#endif

    if (CurrentPowerMode > PowerMode_Lock) {
        EventScheduler_Schedule(CurrentTime + POWER_MODE_RESTART_DELAY, EventSchedulerEvent_PowerModeRestart, "restart power mode");
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
    StateWormhole.rebootToPowerMode = true;
    StateWormhole.restartPowerMode = CurrentPowerMode;
    StateWormhole.magicNumber = WORMHOLE_MAGIC_NUMBER;
    NVIC_SystemReset();
#endif
}

#if DEVICE_IS_KEYBOARD && defined(__ZEPHYR__)

static void runDepletedSleep(bool allowWake) {
    // if the keyboard is powered, wait until it is disconnected
    while (!Charger_ShouldRemainInDepletedMode(false)) {
        if (allowWake && KeyScanner_ScanAndWakeOnSfjl(true, false)) {
            return;
        }
        k_sleep(K_MSEC(PowerModeConfig[CurrentPowerMode].keyScanInterval));
    }

    LogU("Battery is empty. Entering low power mode.\n");

    while (Charger_ShouldRemainInDepletedMode(allowWake)) {
        k_sleep(K_MSEC(5000));
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

void PowerMode_RestartedTo(power_mode_t mode) {
    CurrentPowerMode = mode;
    KeyBacklightBrightness = 0;

    InitLeds_Min();
    InitKeyScanner_Min();
    InitCharger_Min();

    switch (mode) {
        case PowerMode_SfjlSleep:
            runSfjlSleep();
            break;
        case PowerMode_ShutDown:
            runDepletedSleep(false);
            break;
        default:
            break;
    }

    if (DEVICE_IS_UHK80_LEFT) {
        PowerMode_ActivateMode(PowerMode_LightSleep, false);
    } else {
        PowerMode_ActivateMode(PowerMode_Awake, false);
    }
}

#else

void PowerMode_RestartedTo(power_mode_t mode) {};

#endif

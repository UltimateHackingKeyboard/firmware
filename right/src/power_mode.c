#include "power_mode.h"
#include "usb_composite_device.h"
#include "event_scheduler.h"
#include "led_manager.h"

#ifdef __ZEPHYR__
    #include "state_sync.h"
    #include "device_state.h"
    #include "usb/usb.h"
#else
    #include "slave_drivers/is31fl3xxx_driver.h"
    #include "usb_composite_device.h"
    #include "stubs.h"
#endif

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

void PowerMode_Update() {
#ifdef __ZEPHYR__
    bool someoneAwake = usbAwake || DeviceState_IsConnected(ConnectionId_BluetoothHid) || DeviceState_IsConnected(ConnectionId_Dongle);
#else
    bool someoneAwake = CurrentPowerMode == PowerMode_Awake;
#endif

    bool newPowerMode = someoneAwake ? PowerMode_Awake : PowerMode_LightSleep;

    if (CurrentPowerMode <= PowerMode_LightSleep) {
        PowerMode_ActivateMode(newPowerMode, false);
    }
}

static void lightSleep() {
    CurrentPowerMode = PowerMode_LightSleep;

#ifdef __ZEPHYR__
    StateSync_UpdateProperty(StateSyncPropertyId_PowerMode, &CurrentPowerMode);
#endif
    EventVector_Set(EventVector_LedManagerFullUpdateNeeded);
    EventVector_WakeMain();
}

static void deepSleep() {
    CurrentPowerMode = PowerMode_DeepSleep;

    EventVector_Set(EventVector_LedManagerFullUpdateNeeded);
    EventVector_WakeMain();
}


static void wake() {
    CurrentPowerMode = PowerMode_Awake;

#ifdef __ZEPHYR__
    StateSync_UpdateProperty(StateSyncPropertyId_PowerMode, &CurrentPowerMode);
#endif
    EventVector_Set(EventVector_LedManagerFullUpdateNeeded);
    EventVector_WakeMain();
}

void PowerMode_ActivateMode(power_mode_t mode, bool toggle) {
    // if toggling a mode that's currently active, wake up
    if (CurrentPowerMode == mode && toggle) {
        mode = PowerMode_Awake;
    }

    // if two modes are activated, always sink into the deeper of them
    if (mode > PowerMode_Awake && CurrentPowerMode > mode) {
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
        case PowerMode_DeepSleep:
            deepSleep();
            break;
        default:
            break;
    }
}

void PowerMode_WakeHost() {
#ifdef __ZEPHYR__
    USB_RemoteWakeup();
#else
    WakeUpHost();
#endif
}

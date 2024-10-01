#include "power_mode.h"
#include "usb_composite_device.h"
#include "event_scheduler.h"
#include "led_manager.h"

#ifdef __ZEPHYR__
    #include "device_state.h"
    #include "usb/usb.h"
#else
    #include "slave_drivers/is31fl3xxx_driver.h"
    #include "usb_composite_device.h"
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

    bool newPowerMode = someoneAwake ? PowerMode_Awake : PowerMode_Sleep;

    PowerMode_ActivateMode(newPowerMode, false);
}

static void enterSleep() {
    CurrentPowerMode = PowerMode_Sleep;

    EventVector_Set(EventVector_LedManagerFullUpdateNeeded);
    EventVector_WakeMain();
}

void wake() {
    CurrentPowerMode = PowerMode_Awake;

    EventVector_Set(EventVector_LedManagerFullUpdateNeeded);
    EventVector_WakeMain();
}

void PowerMode_ActivateMode(power_mode_t mode, bool toggle) {
    if (CurrentPowerMode == mode && toggle) {
        mode = PowerMode_Awake;
    }

    if (CurrentPowerMode == mode) {
        return;
    }

    switch (mode) {
        case PowerMode_Awake:
            wake();
            break;
        case PowerMode_Sleep:
            enterSleep();
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

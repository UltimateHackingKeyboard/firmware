#include "sleep_mode.h"
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

static bool usbAwake = false;

bool SleepModeActive = true;

// originally written for Benedek's power callback
// TODO: remove this and simplify the rest of the code if the callback is not used.
void SleepMode_SetUsbAwake(bool awake) {
    usbAwake = awake;
#ifdef __ZEPHYR__
    CurrentTime = k_uptime_get();
#endif
    EventScheduler_Reschedule(CurrentTime + SLEEP_MODE_UPDATE_DELAY, EventSchedulerEvent_SleepMode, "update sleep mode from power callback");
}

void SleepMode_Update() {
#ifdef __ZEPHYR__
    bool someoneAwake = usbAwake || DeviceState_IsConnected(ConnectionId_BluetoothHid) || DeviceState_IsConnected(ConnectionId_Dongle);
#else
    bool someoneAwake = SleepModeActive;
#endif

    bool newSleepMode = !someoneAwake;

    if (newSleepMode != SleepModeActive) {
        if (newSleepMode) {
            SleepMode_Enter();
        } else {
            SleepMode_Exit();
        }
    }
}

void SleepMode_Enter() {
    SleepModeActive = true;

    EventVector_Set(EventVector_LedManagerFullUpdateNeeded);
    EventVector_WakeMain();
}

void SleepMode_Exit() {
    SleepModeActive = false;

    EventVector_Set(EventVector_LedManagerFullUpdateNeeded);
    EventVector_WakeMain();
}

void SleepMode_WakeHost() {
#ifdef __ZEPHYR__
    USB_RemoteWakeup();
#else
    WakeUpHost();
#endif
}

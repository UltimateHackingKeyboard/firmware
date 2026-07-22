#include "usb_state.h"
#include "device.h"
#include "power_mode.h"
#include "event_scheduler.h"
#include "usb_report_updater.h"
#include "hid/keyboard_report.h"
#include "stubs.h"

#ifdef __ZEPHYR__
    #include "connections.h"
    #include "state_sync.h"
#endif

#if DEVICE_HAS_OLED
    #include "keyboard/oled/widgets/widget.h"
    #include "keyboard/oled/widgets/widget_store.h"
#endif

// USB transport and host-awake states are kept separately and the derived
// connection state is recalculated whenever either of them changes.
//
// - transport up, but host asleep -> Connected
// - transport up and host awake    -> Ready
// - otherwise                      -> Disconnected
//
// C2usb doesn't report the transport as not available when the host is asleep,
// so the awake state is tracked here on top of the transport state.

bool UsbState_TransportUp = false;
bool UsbState_Awake = true;

static void recalculateConnectionState(void) {
#if DEVICE_IS_UHK_DONGLE
    StateSync_UpdateProperty(StateSyncPropertyId_DongleHostAwake, &UsbState_Awake);
#else
    Connections_SetStateAsync(ConnectionId_UsbHidRight, UsbState_Awake ? ConnectionState_Ready : ConnectionState_Disconnected);
    EventScheduler_Schedule(Timer_GetCurrentTime(), EventSchedulerEvent_PowerModeUpdate, "no host short wakeup");
    WIDGET_REFRESH(&TargetWidget);
#endif
}

void UsbState_SetUsbTransportUp(bool up) {
    if (UsbState_TransportUp != up) {
        UsbState_TransportUp = up;
        recalculateConnectionState();
    }
}

static void probeHostWithReport(void) {
#if !DEVICE_IS_UHK_DONGLE
    GetInactiveKeyboardReport()->modifiers = ~ActiveKeyboardReport->modifiers;

    EventVector_Set(EventVector_SendUsbReports);
    EventVector_WakeMain();
#endif
}

void UsbState_SetUsbAwake(bool awake) {
    if (UsbState_Awake != awake && !awake) {
        UsbState_Awake = awake;
        recalculateConnectionState();
    }
    if (UsbState_Awake != awake && awake) {
        probeHostWithReport();
    }
}

// Remote wakeup doesn't work on some devices. Require a delivered report as a witness.
//
// Do we prefer false positives or false negatives?
void UsbState_Delivered(void) {
    if (!UsbState_Awake) {
        UsbState_Awake = true;
        recalculateConnectionState();
    }
}

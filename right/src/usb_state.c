#include "usb_state.h"
#include "attributes.h"
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

bool UsbState_TransportUp = false;
bool UsbState_HostIsSuspended = false;
bool UsbState_ProvablyAwake = false;

static void recalculateConnectionState(void) {
#if DEVICE_IS_UHK_DONGLE
    // DongleHostAwake keeps awake polarity - it is a state sync wire value shared with
    // the right half, so negate here rather than flipping the protocol.
    bool hostAwake = !UsbState_HostIsSuspended;
    StateSync_UpdateProperty(StateSyncPropertyId_DongleHostAwake, &hostAwake);
#elif defined(__ZEPHYR__)
    Connections_SetStateAsync(ConnectionId_UsbHidRight, UsbState_TransportUp ? ConnectionState_Ready : ConnectionState_Disconnected);
    EventScheduler_Schedule(Timer_GetCurrentTime(), EventSchedulerEvent_PowerModeUpdate, "no host short wakeup");
    WIDGET_REFRESH(&TargetWidget);
#else
    EventScheduler_Schedule(Timer_GetCurrentTime(), EventSchedulerEvent_PowerModeUpdate, "no host short wakeup");
#endif
}

void UsbState_SetUsbTransportUp(bool up) {
    if (UsbState_TransportUp != up) {
        UsbState_TransportUp = up;
        recalculateConnectionState();
    }
}

static ATTR_UNUSED void probeHostWithReport(void) {
#if DEVICE_IS_MASTER
    GetInactiveKeyboardReport()->modifiers = ~ActiveKeyboardReport->modifiers;

    EventVector_Set(EventVector_SendUsbReports);
    EventVector_WakeMain();
#endif
}

void UsbState_SetHostSuspended(bool suspended) {
    if (UsbState_HostIsSuspended != suspended) {
        UsbState_HostIsSuspended = suspended;
        if (suspended) {
            UsbState_ProvablyAwake = false;
        } else {
            // Nothing to probe when the host is gone (detach) or hasn't enumerated us yet.
            // probeHostWithReport();
        }
        recalculateConnectionState();
    }
}

// Remote wakeup doesn't work on some devices. Require a delivered report as a witness.
void UsbState_Delivered(void) {
    if (UsbState_ProvablyAwake) {
        UsbState_ProvablyAwake = true;
        recalculateConnectionState();
    }
}

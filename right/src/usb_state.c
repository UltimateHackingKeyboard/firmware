#include "usb_state.h"
#include "device.h"
#include "power_mode.h"

#ifdef __ZEPHYR__
    #include "connections.h"
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

static bool usbTransportUp = false;
static bool usbAwake = false;

static void recalculateConnectionState(void) {
#if DEVICE_IS_UHK60
    power_mode_t newMode = (usbTransportUp && usbAwake) ? PowerMode_Awake : PowerMode_Uhk60Sleep;
    PowerMode_ActivateMode(newMode, false, false, "received device resume event");
#elif DEVICE_IS_UHK80_RIGHT
    connection_state_t newState;
    if (usbTransportUp && usbAwake) {
        newState = ConnectionState_Ready;
    } else if (usbTransportUp) {
        newState = ConnectionState_Connected;
    } else {
        newState = ConnectionState_Disconnected;
    }
    // This will then trigger power_mode update if the number of awake connections changes.
    Connections_SetStateAsync(ConnectionId_UsbHidRight, newState);
#endif
}

void UsbState_SetUsbTransportUp(bool up) {
    usbTransportUp = up;
    recalculateConnectionState();
}

void UsbState_SetUsbAwake(bool awake) {
    usbAwake = awake;
    recalculateConnectionState();
}

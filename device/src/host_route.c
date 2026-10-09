#include "bt_conn.h"
#include "connections.h"
#include "device.h"
#include "event_scheduler.h"
#include "host_connection.h"
#include "timer.h"
#include "usb_left_relay_uhk.h"
#include "usb_state.h"
#include <stdatomic.h>

#if DEVICE_IS_UHK80_RIGHT
static uint8_t target, phase;
static uint32_t started;
static bool explicitSelection, wasActive, configWaiting, configReady, deferredUsbNeutral;
static uint32_t oldLeftSlots;
static _Atomic uint32_t routeGeneration;
static bool isLeft(uint8_t host)
{
    return Connections_Type(host) == ConnectionType_UsbHidLeft;
}
bool HostRoute_Request(uint8_t host, bool explicitPick)
{
    if (!phase && !isLeft(host) && !isLeft(CurrentHostConnectionId)) {
        if (host != CurrentHostConnectionId) {
            atomic_fetch_add(&routeGeneration, 1);
        }
        return false;
    }
    if (!phase && host == CurrentHostConnectionId && (!isLeft(host) || UsbLeft_Active())) {
        return true;
    }
    atomic_fetch_add(&routeGeneration, 1);
    target = host;
    explicitSelection = explicitPick;
    started = Timer_GetCurrentTime();
    phase = 1;
    UsbLeft_Select(false);
    Hid_BeginNeutralization();
    UsbReportUpdater_ResetHostInput(true);
    EventVector_WakeMain();
    return true;
}
void HostRoute_Process(void)
{
    if (!phase && deferredUsbNeutral && Hid_NeutralizeLocalUsb()) {
        deferredUsbNeutral = false;
        if (Connections_Type(CurrentHostConnectionId) == ConnectionType_UsbHidRight) {
            UsbReportUpdater_ResetHostInput(true);
        }
    }
    bool active = UsbLeft_Active();
    if (wasActive != active && isLeft(CurrentHostConnectionId)) {
        UsbReportUpdater_ResetHostInput(true);
    }
    wasActive = active;
    if (phase == 1) {
        if (!UsbLeft_Quiescent() || !Hid_LocalUsbQuiesce()) {
            return;
        }
        phase = 2;
    }
    if (phase == 2) {
        if (!Hid_NeutralizeCurrentHost()) {
            if (Connections_Type(CurrentHostConnectionId) == ConnectionType_UsbHidRight &&
                UsbState_HostIsSuspended) {
                deferredUsbNeutral =
                    true; // Quiescence was proven in phase 1; only neutral output may follow.
            } else {
                return;
            }
        }
        if (configWaiting) {
            configReady = true;
            phase = 4;
            return;
        }
        phase = 3;
    }
    if (phase == 3) {
        if (isLeft(target)) {
            UsbLeft_Select(true);
            /* An offline explicit pick is still committed; producer output is
             * discarded until negotiation/neutralization can activate it. */
            if (UsbLeft_Connected() && UsbLeft_Awake() && !UsbLeft_Active() &&
                (uint32_t)(Timer_GetCurrentTime() - started) < 100) {
                return;
            }
        }
        UsbReportUpdater_ResetHostInput(true);
        Connections_CommitHost(target, explicitSelection);
        phase = 0;
    }
    if (!phase && isLeft(CurrentHostConnectionId)) {
        UsbLeft_Select(true);
    }
}
bool HostRoute_Blocked(void)
{
    return phase || (isLeft(CurrentHostConnectionId) && !UsbLeft_Active()) ||
           (deferredUsbNeutral &&
               Connections_Type(CurrentHostConnectionId) == ConnectionType_UsbHidRight);
}
bool HostRoute_DiscardInput(void)
{
    return HostRoute_Blocked();
}
bool HostRoute_Pending(void)
{
    return phase != 0;
}
uint8_t HostRoute_Target(void)
{
    return phase ? target : CurrentHostConnectionId;
}
bool HostRoute_Transitioning(void)
{
    return phase && (uint32_t)(Timer_GetCurrentTime() - started) < 100;
}
uint32_t HostRoute_Generation(void)
{
    return atomic_load(&routeGeneration);
}
bool HostRoute_PrepareConfig(void)
{
    if (!UsbLeft_Initialized()) {
        return true;
    }
    if (configReady) {
        configReady = false;
        return true;
    }
    if (!configWaiting) {
        oldLeftSlots = 0;
        for (uint8_t i = 0; i < SERIALIZED_HOST_CONNECTION_COUNT_MAX; ++i) {
            if (HostConnections[i].type == HostConnectionType_UsbHidLeft) {
                oldLeftSlots |= 1u << i;
            }
        }
        atomic_fetch_add(&routeGeneration, 1);
        configWaiting = true;
        target = CurrentHostConnectionId;
        phase = 1;
        started = Timer_GetCurrentTime();
        UsbLeft_Select(false);
        Hid_BeginNeutralization();
        UsbReportUpdater_ResetHostInput(true);
    }
    return false;
}
void HostRoute_ConfigChanged(void)
{
    #ifdef CONFIG_UHK_USB_LEFT_DEV_SLOT
    UsbLeft_Provision();
    #endif
    UsbLeft_Select(false);
    UsbReportUpdater_ResetHostInput(true);
    for (uint8_t i = 0; i < SERIALIZED_HOST_CONNECTION_COUNT_MAX; ++i) {
        uint8_t connection = ConnectionId_HostConnectionFirst + i;
        if ((oldLeftSlots & (1u << i)) &&
            HostConnections[i].type != HostConnectionType_UsbHidLeft &&
            Connections[connection].peerId == PeerIdUnknown) {
            Connections_SetStateAsync(connection, ConnectionState_Disconnected);
        }
    }
    oldLeftSlots = 0;
    configWaiting = configReady = false;
    target = CurrentHostConnectionId;
    if (Connections_Type(target) == ConnectionType_Empty ||
        Connections_Type(target) == ConnectionType_Unknown) {
        for (uint8_t i = 0; i < HOST_CONNECTION_COUNT_MAX; ++i) {
            if (HostConnections[i].type == HostConnectionType_UsbHidRight) {
                target = ConnectionId_HostConnectionFirst + i;
                break;
            }
        }
    }
    explicitSelection = false;
    phase = 3;
    started = Timer_GetCurrentTime();
    Hid_BeginNeutralization();
}
#else
bool HostRoute_Request(uint8_t host, bool explicitPick)
{
    (void)host;
    (void)explicitPick;
    return false;
}
void HostRoute_Process(void) {}
bool HostRoute_Blocked(void)
{
    return false;
}
bool HostRoute_DiscardInput(void)
{
    return false;
}
bool HostRoute_Transitioning(void)
{
    return false;
}
bool HostRoute_Pending(void)
{
    return false;
}
uint8_t HostRoute_Target(void)
{
    return CurrentHostConnectionId;
}
void HostRoute_ConfigChanged(void) {}
uint32_t HostRoute_Generation(void)
{
    return 0;
}
bool HostRoute_PrepareConfig(void)
{
    return true;
}
#endif

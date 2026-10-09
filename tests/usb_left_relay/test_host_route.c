/* Host-side harness for the actual coordinator, with deterministic platform hooks. */
#include "host_input_gate.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define __USB_LEFT_RELAY_UHK_H__
#define __CONNECTIONS_H__
#define __DEVICE_H__
#define __EVENT_SCHEDULER_H__
#define __TIMER_H__
#define __HOST_CONNECTION_H__
#define __BT_CONN_H__
#define __USB_STATE_H__
#define DEVICE_IS_UHK80_RIGHT 1
#define SERIALIZED_HOST_CONNECTION_COUNT_MAX 22
#define HOST_CONNECTION_COUNT_MAX 24
#define ConnectionId_HostConnectionFirst 1
#define PeerIdUnknown 0
#define ConnectionState_Disconnected 0

enum {
    ConnectionType_Unknown,
    ConnectionType_Empty,
    ConnectionType_UsbHidRight,
    ConnectionType_UsbHidLeft,
    ConnectionType_BtHid
};
enum {
    HostConnectionType_Empty = ConnectionType_Empty,
    HostConnectionType_UsbHidRight = ConnectionType_UsbHidRight,
    HostConnectionType_UsbHidLeft = ConnectionType_UsbHidLeft
};
struct {
    uint8_t type;
} HostConnections[HOST_CONNECTION_COUNT_MAX];
struct {
    uint8_t peerId, state;
} Connections[HOST_CONNECTION_COUNT_MAX + 1];
uint8_t CurrentHostConnectionId;
bool UsbState_HostIsSuspended;
static bool available, awake, active, quiescent, drained, neutral, usbNeutral, wanted;
static bool physicalHeld[2];
static uint8_t held[1];
static uint32_t clockMs, commits, resets;
static uint8_t lastCommitted;
static bool lastExplicit;
uint8_t Connections_Type(uint8_t host)
{
    assert(host > 0 && host <= 24);
    return HostConnections[host - 1].type;
}
void Connections_SetStateAsync(uint8_t host, uint8_t state)
{
    Connections[host].state = state;
}
void Connections_CommitHost(uint8_t host, bool explicitPick)
{
    assert(drained && quiescent);
    CurrentHostConnectionId = host;
    ++commits;
    lastCommitted = host;
    lastExplicit = explicitPick;
}
void EventVector_WakeMain(void) {}
uint32_t Timer_GetCurrentTime(void)
{
    return clockMs;
}
bool UsbLeft_Active(void)
{
    return active;
}
bool UsbLeft_Quiescent(void)
{
    return quiescent;
}
bool UsbLeft_Connected(void)
{
    return available;
}
bool UsbLeft_Awake(void)
{
    return awake;
}
bool UsbLeft_Initialized(void)
{
    return true;
}
void UsbLeft_Select(bool selected)
{
    wanted = selected;
}
bool Hid_LocalUsbQuiesce(void)
{
    return drained;
}
bool Hid_NeutralizeCurrentHost(void)
{
    return Connections_Type(CurrentHostConnectionId) == ConnectionType_UsbHidLeft ? quiescent
                                                                                  : neutral;
}
bool Hid_NeutralizeLocalUsb(void)
{
    return usbNeutral;
}
void Hid_BeginNeutralization(void) {}
void UsbReportUpdater_ResetHostInput(bool suppressHeld)
{
    ++resets;
    if (suppressHeld)
        for (uint8_t i = 0; i < 2; ++i)
            HostInputGate_Capture(held, i, physicalHeld[i]);
}

#include "../../device/src/host_route.c"

static void resetPlatform(void)
{
    memset(HostConnections, 0, sizeof(HostConnections));
    memset(Connections, 0, sizeof(Connections));
    HostConnections[0].type = HostConnectionType_UsbHidRight;
    HostConnections[1].type = HostConnectionType_UsbHidLeft;
    CurrentHostConnectionId = 1;
    available = awake = quiescent = drained = neutral = usbNeutral = true;
    active = wanted = UsbState_HostIsSuspended = false;
    memset(held, 0, sizeof(held));
    memset(physicalHeld, 0, sizeof(physicalHeld));
    clockMs = commits = resets = 0;
    target = phase = 0;
    started = 0;
    explicitSelection = wasActive = configWaiting = configReady = deferredUsbNeutral = false;
    oldLeftSlots = 0;
    routeGeneration = 0;
}
static void healthySwitchAndHeldKeys(void)
{
    resetPlatform();
    physicalHeld[0] = true;
    assert(HostRoute_Request(2, true) && CurrentHostConnectionId == 1 && HostRoute_Blocked());
    drained = false;
    clockMs = 20;
    HostRoute_Process();
    assert(commits == 0);
    drained = true;
    neutral = false;
    HostRoute_Process();
    assert(commits == 0);
    neutral = true;
    HostRoute_Process();
    assert(wanted && commits == 0);
    active = true;
    quiescent = true;
    clockMs = 40;
    HostRoute_Process();
    assert(CurrentHostConnectionId == 2 && commits == 1 && lastExplicit && !HostRoute_Blocked());
    assert(HostInputGate_Suppressed(held, 0, false));
    assert(!HostInputGate_Suppressed(
        held, 1, false)); // A distinct key with the same HID usage remains usable.
    assert(!HostInputGate_Suppressed(held, 0, true));
    HostRoute_Process();
    assert(commits == 1);
}
static void offlineAndPreparationDeadline(void)
{
    resetPlatform();
    available = awake = false;
    assert(HostRoute_Request(2, true));
    HostRoute_Process();
    assert(CurrentHostConnectionId == 2 && commits == 1 && HostRoute_Blocked() &&
           !HostRoute_Pending());
    physicalHeld[1] = true;
    available = awake = active = true;
    HostRoute_Process();
    assert(!HostRoute_Blocked() && HostInputGate_Suppressed(held, 1, false));
    resetPlatform();
    assert(HostRoute_Request(2, true));
    HostRoute_Process();
    clockMs = 101;
    HostRoute_Process();
    assert(CurrentHostConnectionId == 2 && HostRoute_Blocked() && !HostRoute_Transitioning());
}
static void latestRequestWinsAndFailedDrain(void)
{
    resetPlatform();
    assert(HostRoute_Request(2, true));
    HostRoute_Process();
    assert(wanted && commits == 0);
    uint32_t oldGeneration = HostRoute_Generation();
    quiescent = false;
    assert(HostRoute_Request(1, true));
    assert(HostRoute_Generation() != oldGeneration);
    HostRoute_Process();
    assert(!wanted && commits == 0 && HostRoute_Target() == 1);
    clockMs = 10000;
    HostRoute_Process();
    assert(
        commits == 0 && HostRoute_Pending() && !HostRoute_Transitioning() && HostRoute_Blocked());
    quiescent = true;
    HostRoute_Process();
    assert(commits == 1 && lastCommitted == 1 && !HostRoute_Blocked());
}
static void sleepingOldUsbKeepsNeutralDebt(void)
{
    resetPlatform();
    neutral = usbNeutral = false;
    UsbState_HostIsSuspended = true;
    assert(HostRoute_Request(2, true));
    HostRoute_Process();
    active = true;
    clockMs = 40;
    HostRoute_Process();
    assert(
        commits == 1 && CurrentHostConnectionId == 2 && deferredUsbNeutral && !HostRoute_Blocked());
    active = false;
    assert(HostRoute_Request(1, true));
    HostRoute_Process();
    assert(CurrentHostConnectionId == 1 && HostRoute_Blocked());
    UsbState_HostIsSuspended = false;
    usbNeutral = true;
    HostRoute_Process();
    assert(!deferredUsbNeutral && !HostRoute_Blocked());
}
static void configReplacementIsBarrier(void)
{
    resetPlatform();
    CurrentHostConnectionId = 2;
    active = true;
    quiescent = false;
    assert(!HostRoute_PrepareConfig());
    HostRoute_Process();
    assert(HostRoute_Blocked() && CurrentHostConnectionId == 2 && !configReady);
    active = false;
    quiescent = true;
    HostRoute_Process();
    assert(configReady && HostRoute_Blocked() && HostRoute_PrepareConfig());
    HostConnections[1].type = HostConnectionType_Empty;
    Connections[2].state = 2;
    HostRoute_ConfigChanged();
    HostRoute_Process();
    assert(CurrentHostConnectionId == 1 && commits == 1 && Connections[2].state == 0 &&
           !HostRoute_Blocked());
}
int main(void)
{
    resetPlatform();
    HostConnections[2].type = ConnectionType_BtHid;
    assert(!HostRoute_Request(3, true));
    assert(HostRoute_Generation() == 1 && !HostRoute_Pending());
    CurrentHostConnectionId = 3;
    assert(!HostRoute_Request(1, true));
    assert(HostRoute_Generation() == 2); // Legacy-route callbacks cannot advance a new route.
    healthySwitchAndHeldKeys();
    offlineAndPreparationDeadline();
    latestRequestWinsAndFailedDrain();
    sleepingOldUsbKeepsNeutralDebt();
    configReplacementIsBarrier();
    puts("host route tests passed");
}

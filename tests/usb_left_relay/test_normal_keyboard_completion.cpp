#include <cassert>
#include <cstdio>
#include <cstring>
#include <span>
#include <cerrno>
#include "usb_left_transfer.h"

// Real adapter callbacks are extracted by the runner. Platform and report
// construction are stubbed so a timeout/rebuild can be scheduled deterministically.
#define DEVICE_IS_UHK80_RIGHT 1
enum { RelayKind_Keyboard = 1, RelayKind_Mouse = 2, RelayKind_Controls = 3, RelayKind_Count = 3 };
enum { ConnectionType_UsbHidRight, ConnectionType_BtHid };
enum { ReportSink_Usb, ReportSink_BleHid };
struct hid_keyboard_report_t { uint8_t modifiers, bitfield[28]; };
struct report_send_state_t { bool inFlight; };
static struct { report_send_state_t keyboard, mouse, controls; } UsbSemaphore;
static struct k_spinlock {} ticketLock;
static int k_spin_lock(k_spinlock *) { return 0; }
static void k_spin_unlock(k_spinlock *, int) {}
static uint32_t localUsbGeneration = 19, route = 4;
[[maybe_unused]] static uint32_t localUsbFence;
static uint32_t atomic_get(uint32_t *p) { return *p; }
static uint32_t Hid_LocalUsbGeneration() { return localUsbGeneration; }
static uint32_t HostRoute_Generation() { return route; }
static bool blocked;
static bool HostRoute_Blocked() { return blocked; }
static uint8_t CurrentHostConnectionId = 1;
static int currentType = ConnectionType_UsbHidRight;
static int Connections_Type(uint8_t) { return currentType; }
namespace hid {
enum class channel { USB, BLE };
struct session {
    hid::channel channel() { return hid::channel::USB; }
    struct result { int to_int() { return 0; } };
    result send_report(std::span<const uint8_t>) { return {}; }
};
}
static hid::session session;
static void *UsbLeft_CurrentHostBleSession() { return &session; }
static relay_transfer_ticket_t tickets[4];
static struct {
    relay_transfer_ticket_t ticket;
    bool ready, success;
    hid_keyboard_report_t keyboard;
} normalCompletions[4];
static hid_keyboard_report_t normalKeyboardSnapshot;
[[maybe_unused]] static uint8_t canonicalUsbReports[4][29];
[[maybe_unused]] static bool Hid_LocalUsbQueueSend(uint8_t) { return true; }
static uint8_t neutralDone, neutralSubmitted, usbNeutralDone, usbNeutralSubmitted;
static hid_keyboard_report_t reports[2];
static unsigned active;
static unsigned delivered;
static hid_keyboard_report_t *GetInactiveKeyboardReport() { return &reports[active ^ 1]; }
[[maybe_unused]] static void UsbSemaphore_Confirm(report_send_state_t *st) { st->inFlight = false; }
static void UsbSemaphore_Release(report_send_state_t *st) { active ^= 1; st->inFlight = false; }
static void UsbScheduler_ReportDelivered(int) { ++delivered; }
static void UsbState_Delivered() {}
static void EventVector_WakeMain() {}
static void UsbLeft_LocalComplete(uint8_t, uint64_t, uint32_t, bool) {}
#include "normal_completion_under_test.inc"

static uint8_t payload[30];
static void submit(bool usb = true) {
#ifdef NORMAL_SNAPSHOT_SUPPORT
    if (usb) {
        assert(queueUsb(1, payload, sizeof(payload), localUsbGeneration, 0, 0,
                   false, false, false, &reports[active]) == 0);
        auto reservation = tickets[1];
        assert(trackedSend(1, &session, payload, 0, 0, false, false, route, &reservation) == 0);
    } else {
        assert(trackedSend(1, &session, payload, 0, 0, false, false, route,
                   nullptr, &reports[active]) == 0);
        tickets[1].usb = false; // Fake session is selected as BLE for this case.
    }
#else
    normalKeyboardSnapshot = reports[active];
    tickets[RelayKind_Keyboard] = {};
    auto &ticket = tickets[RelayKind_Keyboard];
    ticket.session = &session;
    ticket.data = payload;
    ticket.size = sizeof(payload);
    ticket.pending = true;
    ticket.usb = usb;
    ticket.generation = localUsbGeneration;
    ticket.route = route;
#endif
    UsbSemaphore.keyboard.inFlight = true;
}
static void complete() {
    Hid_LocalUsbComplete(RelayKind_Keyboard, &session, payload, sizeof(payload));
}
static void reset() {
    std::memset(reports, 0, sizeof(reports));
    std::memset(tickets, 0, sizeof(tickets));
    std::memset(normalCompletions, 0, sizeof(normalCompletions));
    UsbSemaphore = {};
    localUsbFence = 0;
    active = delivered = 0;
    blocked = false;
    currentType = ConnectionType_UsbHidRight;
}
int main() {
    // Exact observed discrepancy: Backspace-down delivered, canonical buffers up.
    reset();
    reports[active].bitfield[4] = 0x40;
    submit();
    UsbSemaphore.keyboard.inFlight = false; // semaphore timeout / failed retry
    reports[active] = {}; // desired state rebuilt after physical release
    complete();
#ifdef NORMAL_SNAPSHOT_SUPPORT
    // A callback waiting for main-thread drain still owns its delivery snapshot.
    assert(queueUsb(1, payload, sizeof(payload), localUsbGeneration, 0, 0,
               false, false, false, &reports[active]) == -EBUSY);
    assert(trackedSend(1, &session, payload, 0, 0, false, false, route,
               nullptr, &reports[active]) == -EBUSY);
#endif
    Hid_LocalUsbDrainCompletions();
    assert(delivered == 1);
    assert(GetInactiveKeyboardReport()->bitfield[4] == 0x40);
    assert(reports[active].bitfield[4] == 0);
    assert(std::memcmp(&reports[0], &reports[1], sizeof(reports[0])) != 0);
    // That difference forces the next report to deliver the release.
    submit();
    complete();
    Hid_LocalUsbDrainCompletions();
    assert(delivered == 2);
    assert(std::memcmp(&reports[0], &reports[1], sizeof(reports[0])) == 0);

    // Even with the latch set, completion cannot promote mutable desired input.
    reset();
    reports[active].bitfield[4] = 0x40;
    submit();
    reports[active] = {};
    complete();
    Hid_LocalUsbDrainCompletions();
    assert(GetInactiveKeyboardReport()->bitfield[4] == 0x40);
    assert(reports[active].bitfield[4] == 0);

    // New routing/session generations must reject stale delivery baselines.
    reset(); submit(); complete(); ++route;
    Hid_LocalUsbDrainCompletions(); assert(delivered == 0);
    reset(); submit(); complete(); ++localUsbGeneration;
    Hid_LocalUsbDrainCompletions(); assert(delivered == 0);
    reset(); submit(); complete(); blocked = true;
    Hid_LocalUsbDrainCompletions(); assert(delivered == 0);

    // A cancelled completion never acknowledges a key or updates its baseline.
    reset(); submit(); tickets[1].cancelling = true; complete();
    Hid_LocalUsbDrainCompletions(); assert(delivered == 0);

    // The same late-ACK contract applies to the selected BLE session.
    reset(); currentType = ConnectionType_BtHid;
    reports[active].bitfield[4] = 0x40; submit(false);
    UsbSemaphore.keyboard.inFlight = false; reports[active] = {};
    complete(); Hid_LocalUsbDrainCompletions();
    assert(delivered == 1 && GetInactiveKeyboardReport()->bitfield[4] == 0x40);
    std::puts("normal keyboard completion regression passed");
}

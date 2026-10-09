#include "usb_left_adapter.hpp"
#include "controls_app.hpp"
#include "keyboard_app.hpp"
#include "mouse_app.hpp"
extern "C" {
#include "connections.h"
#include "event_scheduler.h"
#include "link_protocol.h"
#include "messenger.h"
#include "state_sync.h"
#include "usb_left_relay_uhk.h"
#include "usb_left_transfer.h"
#include "usb_scheduler.h"
#include "usb_semaphore.h"
#include "usb_report_updater.h"
#include "usb_state.h"
}
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#ifdef CONFIG_UHK_USB_LEFT_RELAY
    /* Transfer identity includes the session, immutable buffer and expected length.
 * Cancellation never releases a new route's semaphore. */
    #if DEVICE_IS_UHK80_RIGHT
hid::session *UsbLeft_CurrentHostBleSession();
    #endif
static struct k_spinlock ticketLock;
static atomic_t localUsbGeneration, localUsbFence;
static relay_transfer_ticket_t tickets[RelayKind_Count + 1];
static struct {
    relay_transfer_ticket_t ticket;
    bool ready, success;
    hid_keyboard_report_t keyboard;
} normalCompletions[RelayKind_Count + 1];
static hid_keyboard_report_t normalKeyboardSnapshot;
static uint8_t neutralDone, neutralSubmitted, usbNeutralDone, usbNeutralSubmitted;
static key_report_buffer relayKeyboardBuffer;
static double_buffer<mouse_app::mouse_report_base<report_ids::IN_MOUSE>> relayMouseBuffer;
static double_buffer<controls_app::controls_report_base<report_ids::IN_CONTROLS>>
    relayControlsBuffer;

static int trackedSend(uint8_t kind, hid::session *session, std::span<const uint8_t> payload,
    uint64_t token = 0, uint32_t sequence = 0, bool relay = false, bool neutral = false,
    uint32_t route = HostRoute_Generation(), const relay_transfer_ticket_t *reservation = nullptr,
    const hid_keyboard_report_t *keyboard = nullptr)
{
    if (!session) {
        return -ENOTCONN;
    }
    if (session->channel() == hid::channel::USB && atomic_get(&localUsbFence)) {
        return -EAGAIN;
    }
    auto key = k_spin_lock(&ticketLock);
    if (reservation && (!RelayTransfer_IsReservation(&tickets[kind], reservation) ||
                           tickets[kind].cancelling || atomic_get(&localUsbFence) ||
                           reservation->generation != uint32_t(atomic_get(&localUsbGeneration)))) {
        k_spin_unlock(&ticketLock, key);
        return -ECANCELED;
    }
    if ((tickets[kind].pending && !reservation) ||
        (kind == RelayKind_Keyboard && normalCompletions[kind].ready)) {
        k_spin_unlock(&ticketLock, key);
        return -EBUSY;
    }
    tickets[kind] = {session, payload.data(), payload.size(), token, sequence,
        uint32_t(atomic_get(&localUsbGeneration)), true, false, relay, neutral,
        session->channel() == hid::channel::USB, false, false, false, route};
    if (keyboard) {
        normalKeyboardSnapshot = *keyboard;
    }
    k_spin_unlock(&ticketLock, key);
    int result = session->send_report(payload).to_int();
    if (result) {
        key = k_spin_lock(&ticketLock);
        tickets[kind].pending = false;
        k_spin_unlock(&ticketLock, key);
    }
    return result;
}
extern "C" void Hid_LocalUsbComplete(
    uint8_t kind, const void *session, const void *data, size_t size)
{
    auto key = k_spin_lock(&ticketLock);
    relay_transfer_ticket_t ticket;
    auto result = RelayTransfer_Complete(
        &tickets[kind], session, data, size, atomic_get(&localUsbGeneration), &ticket);
    if (result == RelayTransfer_Ignored) {
        k_spin_unlock(&ticketLock, key);
        return;
    }
    bool success = result == RelayTransfer_Success;
    if (ticket.neutral) {
        auto &done = ticket.usb ? usbNeutralDone : neutralDone;
        auto &submitted = ticket.usb ? usbNeutralSubmitted : neutralSubmitted;
        if (success) {
            done |= 1u << kind;
        } else {
            submitted &= ~(1u << kind);
        }
    }
    if (!ticket.relay && !ticket.neutral) {
        normalCompletions[kind] = {ticket, true, success, normalKeyboardSnapshot};
    }
    k_spin_unlock(&ticketLock, key);
    if (ticket.relay) {
        UsbLeft_LocalComplete(kind, ticket.token, ticket.sequence, success);
    }
    EventVector_WakeMain();
}
extern "C" void Hid_LocalUsbDrainCompletions(void)
{
    #if DEVICE_IS_UHK80_RIGHT
    for (uint8_t kind = 1; kind <= RelayKind_Count; ++kind) {
        auto key = k_spin_lock(&ticketLock);
        auto completion = normalCompletions[kind];
        normalCompletions[kind].ready = false;
        k_spin_unlock(&ticketLock, key);
        if (!completion.ready || !completion.success ||
            completion.ticket.route != HostRoute_Generation() || HostRoute_Blocked()) {
            continue;
        }
        const auto &ticket = completion.ticket;
        if (ticket.usb) {
            if (ticket.generation != Hid_LocalUsbGeneration() ||
                Connections_Type(CurrentHostConnectionId) != ConnectionType_UsbHidRight) {
                continue;
            }
        } else if (ticket.session != UsbLeft_CurrentHostBleSession()) {
            continue;
        }
        report_send_state_t *states[] = {
            &UsbSemaphore.keyboard, &UsbSemaphore.mouse, &UsbSemaphore.controls};
        if (kind == RelayKind_Keyboard || states[kind - 1]->inFlight) {
            if (kind == RelayKind_Keyboard) {
                /* The active report may have been rebuilt after a timeout.
                 * Record what actually reached the host, including late ACKs,
                 * without turning the mutable desired state into the baseline. */
                *GetInactiveKeyboardReport() = completion.keyboard;
                UsbSemaphore_Confirm(states[kind - 1]);
            } else {
                UsbSemaphore_Release(states[kind - 1]);
            }
            UsbScheduler_ReportDelivered(ticket.usb ? ReportSink_Usb : ReportSink_BleHid);
            if (ticket.usb) {
                UsbState_Delivered();
            }
        }
    }
    #endif
}
extern "C" uint32_t Hid_LocalUsbGeneration(void)
{
    return atomic_get(&localUsbGeneration);
}
extern "C" void Hid_LocalUsbSessionChanged(void)
{
    atomic_set(&localUsbFence, 1);
    atomic_inc(&localUsbGeneration);
    Hid_LocalUsbRequestFence();
    EventVector_WakeMain();
}
extern "C" void Hid_LocalUsbConfigurationChanged(void)
{
    Hid_LocalUsbSessionChanged();
}
extern "C" uint8_t Hid_LocalUsbRetireTickets(void)
{
    uint8_t mask = 0;
    auto key = k_spin_lock(&ticketLock);
    for (uint8_t kind = 1; kind <= RelayKind_Count; ++kind) {
        auto &ticket = tickets[kind];
        if (ticket.pending && ticket.usb) {
            ticket.cancelling = true;
            mask |= 1u << kind;
        }
    }
    k_spin_unlock(&ticketLock, key);
    return mask;
}
extern "C" void Hid_LocalUsbFinishFence(uint32_t generation)
{
    /* The USB worker posts this behind all cancellation completions, after
     * the driver has confirmed DMA abort. No storage is reused before it. */
    auto key = k_spin_lock(&ticketLock);
    if (generation == uint32_t(atomic_get(&localUsbGeneration))) {
        for (auto &ticket : tickets) {
            if (ticket.usb) {
                ticket.pending = false;
            }
        }
        usbNeutralDone = usbNeutralSubmitted = 0;
        atomic_set(&localUsbFence, 0);
    }
    k_spin_unlock(&ticketLock, key);
    EventVector_WakeMain();
}
extern "C" bool Hid_LocalUsbCancellationPending(uint8_t kind, uint32_t *generation)
{
    if (!kind || kind > RelayKind_Count) {
        return false;
    }
    auto key = k_spin_lock(&ticketLock);
    bool pending = tickets[kind].pending && tickets[kind].cancelling && tickets[kind].usb;
    *generation = tickets[kind].generation;
    k_spin_unlock(&ticketLock, key);
    return pending;
}
extern "C" bool Hid_LocalUsbQuiesce(void)
{
    if (atomic_get(&localUsbFence)) {
        return false;
    }
    bool pending = false;
    for (uint8_t kind = 1; kind <= RelayKind_Count; ++kind) {
        auto key = k_spin_lock(&ticketLock);
        bool cancel = tickets[kind].pending && !tickets[kind].cancelQueued && tickets[kind].usb;
    #if DEVICE_IS_UHK80_RIGHT
        if (tickets[kind].pending && !tickets[kind].usb && !UsbLeft_CurrentHostBleSession()) {
            tickets[kind].pending = false;
        }
    #endif
        if (tickets[kind].pending) {
            tickets[kind].cancelling = true;
            if (cancel) {
                tickets[kind].cancelQueued = true;
            }
            pending = true;
        }
        k_spin_unlock(&ticketLock, key);
        if (cancel && !Hid_LocalUsbCancel(kind)) {
            key = k_spin_lock(&ticketLock);
            tickets[kind].cancelQueued = false;
            k_spin_unlock(&ticketLock, key);
        }
    }
    return !pending;
}
static int submitLocalUsbOnWorker(uint8_t kind, const uint8_t *canonical, uint64_t token,
    uint32_t sequence, bool relay, bool neutral, const relay_transfer_ticket_t *reservation)
{
    if (!kind || kind > RelayKind_Count) {
        return -EINVAL;
    }
    if (atomic_get(&localUsbFence) || !UsbState_TransportUp || UsbState_HostIsSuspended) {
        return -EHOSTUNREACH;
    }
    std::span<const uint8_t> payload;
    hid::session *session = nullptr;
    /* Do not touch an encoder whose previous transfer still owns its storage. */
    auto key = k_spin_lock(&ticketLock);
    bool busy = tickets[kind].pending && !RelayTransfer_IsReservation(&tickets[kind], reservation);
    k_spin_unlock(&ticketLock, key);
    if (busy) {
        return -EBUSY;
    }
    if (kind == RelayKind_Keyboard) {
        auto kb = keyboard_app::usb_handle().session();
        if (!kb) {
            return -ENOTCONN;
        }
        relayKeyboardBuffer.reset_to(kb->protocol(), HID_GetKeyboardRollover());
        hid_keyboard_report_t report;
        memcpy(&report, canonical, sizeof(report));
        payload = relayKeyboardBuffer.insert(report);
        session = kb;
    } else if (kind == RelayKind_Mouse) {
        hid_mouse_report_t report{};
        report.buttons =
            canonical[0] | (uint32_t(canonical[1]) << 8) | (uint32_t(canonical[2]) << 16);
        auto get16 = [](const uint8_t *p) {
            return int16_t(uint16_t(p[0]) | (uint16_t(p[1]) << 8));
        };
        report.x = get16(canonical + 3);
        report.y = get16(canonical + 5);
        report.wheelY = get16(canonical + 7);
        report.wheelX = get16(canonical + 9);
        payload = relayMouseBuffer.insert(report);
        session = mouse_app::usb_handle().session();
    } else if (kind == RelayKind_Controls) {
        hid_controls_report_t report{};
        for (uint8_t i = 0; i < 4; ++i) {
            report.consumer[i] = canonical[2 * i] | (uint16_t(canonical[2 * i + 1]) << 8);
        }
        report.system[0] = canonical[8];
        report.system[1] = canonical[9];
        payload = relayControlsBuffer.insert(report);
        session = controls_app::usb_handle().session();
    } else {
        return -EINVAL;
    }
    int result = trackedSend(
        kind, session, payload, token, sequence, relay, neutral, reservation->route, reservation);
    if (!result) {
        if (kind == RelayKind_Keyboard) {
            relayKeyboardBuffer.swap_sides();
        } else if (kind == RelayKind_Mouse) {
            relayMouseBuffer.swap_sides();
        } else {
            relayControlsBuffer.swap_sides();
        }
    }
    return result;
}
static uint8_t canonicalUsbReports[RelayKind_Count + 1][USB_LEFT_RELAY_KEYBOARD_SIZE];
static void failedQueuedUsb(uint8_t kind, const relay_transfer_ticket_t &ticket)
{
    auto key = k_spin_lock(&ticketLock);
    if (RelayTransfer_IsReservation(&tickets[kind], &ticket)) {
        tickets[kind].pending = false;
    }
    k_spin_unlock(&ticketLock, key);
    if (ticket.relay) {
        UsbLeft_LocalComplete(kind, ticket.token, ticket.sequence, false);
    }
    if (ticket.neutral) {
        auto key = k_spin_lock(&ticketLock);
        usbNeutralSubmitted &= ~(1u << kind);
        k_spin_unlock(&ticketLock, key);
    }
    EventVector_WakeMain();
}
static int queueUsb(uint8_t kind, const uint8_t *data, size_t size, uint32_t generation,
    uint64_t token, uint32_t sequence, bool relay, bool neutral, bool canonical,
    const hid_keyboard_report_t *keyboard = nullptr)
{
    auto key = k_spin_lock(&ticketLock);
    if (tickets[kind].pending || (kind == RelayKind_Keyboard && normalCompletions[kind].ready) ||
        atomic_get(&localUsbFence) ||
        generation != uint32_t(atomic_get(&localUsbGeneration))) {
        k_spin_unlock(&ticketLock, key);
        return -EBUSY;
    }
    if (canonical) {
        memcpy(canonicalUsbReports[kind], data, size);
        data = canonicalUsbReports[kind];
    }
    tickets[kind] = {nullptr, data, size, token, sequence, generation, true, false, relay, neutral,
        true, false, true, canonical, HostRoute_Generation()};
    if (keyboard) {
        normalKeyboardSnapshot = *keyboard;
    }
    k_spin_unlock(&ticketLock, key);
    if (!Hid_LocalUsbQueueSend(kind)) {
        key = k_spin_lock(&ticketLock);
        tickets[kind].pending = false;
        k_spin_unlock(&ticketLock, key);
        return -EAGAIN;
    }
    return 0;
}
extern "C" void Hid_LocalUsbSendQueued(uint8_t kind)
{
    auto key = k_spin_lock(&ticketLock);
    auto ticket = tickets[kind];
    if (!ticket.pending || !ticket.queued) {
        k_spin_unlock(&ticketLock, key);
        return;
    }
    k_spin_unlock(&ticketLock, key);
    if (ticket.cancelling || atomic_get(&localUsbFence) ||
        ticket.generation != uint32_t(atomic_get(&localUsbGeneration))) {
        failedQueuedUsb(kind, ticket);
        return;
    }
    int result;
    if (ticket.canonical) {
        result = submitLocalUsbOnWorker(kind, ticket.data, ticket.token, ticket.sequence,
            ticket.relay, ticket.neutral, &ticket);
    } else {
        hid::session *session =
            kind == RelayKind_Keyboard
                ? static_cast<hid::session *>(keyboard_app::usb_handle().session())
            : kind == RelayKind_Mouse
                ? static_cast<hid::session *>(mouse_app::usb_handle().session())
                : controls_app::usb_handle().session();
        result = trackedSend(kind, session, std::span<const uint8_t>(ticket.data, ticket.size), 0,
            0, false, false, ticket.route, &ticket);
    }
    if (result) {
        failedQueuedUsb(kind, ticket);
    }
}
extern "C" int Hid_LocalUsbSubmit(
    uint8_t kind, const uint8_t *canonical, uint64_t token, uint32_t sequence, uint32_t generation)
{
    size_t size = RelayProtocol_ReportSize(kind);
    if (!size) {
        return -EINVAL;
    }
    return queueUsb(kind, canonical, size, generation, token, sequence, true, false, true);
}
extern "C" bool Hid_NeutralizeLocalUsb(void)
{
    if (!UsbState_TransportUp && !atomic_get(&localUsbFence)) {
        return true;
    }
    uint8_t zero[USB_LEFT_RELAY_KEYBOARD_SIZE] = {};
    for (uint8_t kind = 1; kind <= RelayKind_Count; ++kind) {
        auto key = k_spin_lock(&ticketLock);
        bool submitted = usbNeutralSubmitted & (1u << kind);
        if (!submitted) {
            usbNeutralSubmitted |= 1u << kind;
        }
        k_spin_unlock(&ticketLock, key);
        if (!submitted && queueUsb(kind, zero, RelayProtocol_ReportSize(kind),
                              Hid_LocalUsbGeneration(), 0, 0, false, true, true)) {
            key = k_spin_lock(&ticketLock);
            usbNeutralSubmitted &= ~(1u << kind);
            k_spin_unlock(&ticketLock, key);
        }
    }
    auto key = k_spin_lock(&ticketLock);
    bool done = usbNeutralDone == 14;
    k_spin_unlock(&ticketLock, key);
    return done;
}
extern "C" void Hid_LocalUsbSnapshot(uint32_t *generation, relay_usb_state_t *state)
{
    *generation = atomic_get(&localUsbGeneration);
    *state = {};
    state->flags =
        (UsbState_TransportUp && !atomic_get(&localUsbFence) ? RelayFlag_Configured : 0) |
        (UsbState_HostIsSuspended ? RelayFlag_Suspended : 0) |
        (Hid_LocalUsbWakeAllowed() ? RelayFlag_Wakeup : 0) |
        (USB_IsMsHost() ? RelayFlag_Windows : 0);
    state->rollover = HID_GetKeyboardRollover();
    if (auto session = keyboard_app::usb_handle().session(); session) {
        state->protocol = uint8_t(session->protocol());
        auto report = session->get_leds_report();
        state->leds = (report.leds.test(hid::page::leds::NUM_LOCK) ? 1 : 0) |
                      (report.leds.test(hid::page::leds::CAPS_LOCK) ? 2 : 0) |
                      (report.leds.test(hid::page::leds::SCROLL_LOCK) ? 4 : 0);
    }
    state->vertical = state->horizontal = 256;
    if (auto session = mouse_app::usb_handle().session(); session) {
        state->vertical = uint16_t(session->resolution_report().vertical_scroll_multiplier() * 256);
        state->horizontal =
            uint16_t(session->resolution_report().horizontal_scroll_multiplier() * 256);
    }
}
#endif

#ifdef CONFIG_UHK_USB_LEFT_RELAY
extern "C" void Hid_BeginNeutralization(void)
{
    auto key = k_spin_lock(&ticketLock);
    neutralDone = neutralSubmitted = usbNeutralDone = usbNeutralSubmitted = 0;
    k_spin_unlock(&ticketLock, key);
}
extern "C" bool Hid_NeutralizeCurrentHost(void)
{
    #if DEVICE_IS_UHK80_RIGHT
    auto type = Connections_Type(CurrentHostConnectionId);
    if (type == ConnectionType_UsbHidLeft) {
        return UsbLeft_Quiescent();
    }
    if (type == ConnectionType_UsbHidRight) {
        return Hid_NeutralizeLocalUsb();
    }
    if (type == ConnectionType_BtHid && !UsbLeft_CurrentHostBleSession()) {
        return true;
    }
    if (!Connections_IsReady(CurrentHostConnectionId)) {
        return false;
    }
    uint8_t zero[USB_LEFT_RELAY_KEYBOARD_SIZE] = {};
    for (uint8_t kind = 1; kind <= RelayKind_Count; ++kind) {
        auto key = k_spin_lock(&ticketLock);
        bool submitted = neutralSubmitted & (1u << kind);
        if (!submitted) {
            neutralSubmitted |= 1u << kind;
        }
        k_spin_unlock(&ticketLock, key);
        if (submitted) {
            continue;
        }
        int result = -EHOSTUNREACH;
        if (type == ConnectionType_NusDongle) {
            uint8_t id = kind == RelayKind_Keyboard ? SyncablePropertyId_KeyboardReport
                         : kind == RelayKind_Mouse  ? SyncablePropertyId_MouseReport
                                                    : SyncablePropertyId_ControlsReport;
            result = Messenger_Send2(DeviceId_Uhk_Dongle, MessageId_SyncableProperty, id, zero,
                RelayProtocol_ReportSize(kind));
            if (!result) {
                key = k_spin_lock(&ticketLock);
                neutralDone |= 1u << kind;
                k_spin_unlock(&ticketLock, key);
            }
        } else if (type == ConnectionType_BtHid) {
            auto session = UsbLeft_CurrentHostBleSession();
            std::span<const uint8_t> payload;
            if (kind == RelayKind_Keyboard) {
                relayKeyboardBuffer.reset_to(session->protocol(), ROLLOVER_N_KEY);
                hid_keyboard_report_t report{};
                payload = relayKeyboardBuffer.insert(report);
            } else if (kind == RelayKind_Mouse) {
                hid_mouse_report_t report{};
                payload = relayMouseBuffer.insert(report);
            } else {
                hid_controls_report_t report{};
                payload = relayControlsBuffer.insert(report);
            }
            result = trackedSend(kind, session, payload, UINT64_MAX, 0, false, true);
        } else {
            return true;
        }
        if (result) {
            key = k_spin_lock(&ticketLock);
            neutralSubmitted &= ~(1u << kind);
            k_spin_unlock(&ticketLock, key);
        }
    }
    auto key = k_spin_lock(&ticketLock);
    bool done = neutralDone == 14;
    k_spin_unlock(&ticketLock, key);
    return done;
    #else
    return true;
    #endif
}
#endif

bool UsbLeft_KeyboardPending()
{
    auto key = k_spin_lock(&ticketLock);
    bool pending = tickets[RelayKind_Keyboard].pending || normalCompletions[RelayKind_Keyboard].ready;
    k_spin_unlock(&ticketLock, key);
    return pending;
}
bool UsbLeft_QuiesceKeyboardProtocol()
{
    auto key = k_spin_lock(&ticketLock);
    bool pending = tickets[RelayKind_Keyboard].pending;
    bool usb = tickets[RelayKind_Keyboard].usb;
    if (pending) {
        tickets[RelayKind_Keyboard].cancelling = true;
    }
    k_spin_unlock(&ticketLock, key);
    if (pending && usb) {
        (void)Hid_LocalUsbQuiesce();
    }
    return pending;
}
int UsbLeft_TrackedSend(uint8_t kind, hid::session *session, std::span<const uint8_t> data,
    const hid_keyboard_report_t *keyboard)
{
    return trackedSend(kind, session, data, 0, 0, false, false, HostRoute_Generation(), nullptr, keyboard);
}
int UsbLeft_QueueUsb(uint8_t kind, std::span<const uint8_t> data, uint32_t generation,
    const hid_keyboard_report_t *keyboard)
{
    return queueUsb(kind, data.data(), data.size(), generation, 0, 0, false, false, false, keyboard);
}

#include "usb_left_relay.h"
#include <string.h>

enum {
    Cleanup_None,
    Cleanup_Begin,
    Cleanup_End,
    Cleanup_Fault
};

static bool elapsed(uint32_t now, uint32_t then, uint32_t interval)
{
    return now - then >= interval;
}
static void changed(relay_t *r)
{
    if (r->platform.changed)
        r->platform.changed(r->user);
}
static uint64_t nonce(relay_t *r)
{
    uint64_t n = r->platform.nonce(r->user);
    return n ? n : 1;
}
static bool sendMessage(relay_t *r, const relay_message_t *m)
{
    uint8_t packet[USB_LEFT_RELAY_MAX_PACKET];
    size_t size = RelayProtocol_Encode(m, packet, sizeof(packet));
    return r->link && size && r->platform.send(r->user, packet, size);
}
static void stateMessage(relay_t *r, relay_opcode_t op)
{
    relay_message_t m = {
        .op = op,
        .right_boot = r->peer_boot,
        .left_boot = r->boot,
        .challenge = r->challenge,
        .usb_generation = r->usb_generation,
        .schema = USB_LEFT_RELAY_SCHEMA,
        .capabilities = RelayCaps_All,
        .state = r->usb,
    };
    m.state.flags &= ~(RelayFlag_Active | RelayFlag_Fault);
    if (r->active)
        m.state.flags |= RelayFlag_Active;
    if (r->reason)
        m.state.flags |= RelayFlag_Fault;
    sendMessage(r, &m);
    r->last_state = r->now;
}
static void result(
    relay_t *r, relay_opcode_t op, uint64_t token, uint8_t kind, uint32_t seq, uint8_t status)
{
    relay_message_t m = {.op = op,
        .token = token,
        .kind = kind,
        .sequence = seq,
        .status = status,
        .selection = r->accepted_selection,
        .usb_generation = r->usb_generation};
    sendMessage(r, &m);
}
static void clearPending(relay_t *r)
{
    for (size_t i = 0; i < RelayKind_Count; ++i) {
        r->slots[i].pending = false;
        r->slots[i].submitted = false;
    }
}
static void leftCleanup(relay_t *r, uint8_t cleanup, uint8_t reason)
{
    r->active = false;
    r->cleanup = cleanup;
    r->reason = reason;
    r->neutral_mask = 0;
    r->quiescent = false;
    clearPending(r);
    ++r->usb.revision;
    if (cleanup == Cleanup_Fault) {
        ++r->faults;
        result(r, RelayOp_Fault, r->token, 0, 0, reason);
    }
    if (r->negotiated)
        stateMessage(r, RelayOp_State);
    changed(r);
}
static void rightClose(relay_t *r, uint8_t reason)
{
    r->active = false;
    r->starting = false;
    r->reason = reason;
    clearPending(r);
    r->ending = r->token != 0;
    r->quiescent = !r->ending;
    r->control_started = r->now;
    r->last_control = r->now - RELAY_REPORT_RETRY_MS;
    if (reason)
        ++r->faults;
    changed(r);
}

void Relay_Init(relay_t *r, bool right, const relay_platform_t *platform, void *user)
{
    memset(r, 0, sizeof(*r));
    r->platform = *platform;
    r->user = user;
    r->right = right;
    r->boot = nonce(r);
    r->quiescent = true;
    r->usb.vertical = r->usb.horizontal = 256;
}

void Relay_SetLink(relay_t *r, bool up)
{
    if (r->link == up)
        return;
    r->link = up;
    r->negotiated = false;
    r->hello_attempts = 0;
    r->last_hello = r->now;
    r->wake_sent = false;
    if (r->right) {
        rightClose(r, r->token ? RelayStatus_Unavailable : 0);
    } else {
        r->challenge = nonce(r);
        r->peer_boot = 0;
        r->accepted_selection = 0;
        if (r->token || !r->quiescent)
            leftCleanup(r, Cleanup_End, RelayStatus_Unavailable);
    }
    changed(r);
}

bool Relay_Available(const relay_t *r)
{
    return r->link && r->negotiated && (r->usb.flags & RelayFlag_Configured) &&
           (!r->right || !elapsed(r->now, r->last_rx, 1000));
}
bool Relay_Awake(const relay_t *r)
{
    return Relay_Available(r) && !(r->usb.flags & RelayFlag_Suspended);
}

void Relay_SetUsb(relay_t *r, uint32_t generation, const relay_usb_state_t *usb)
{
    if (r->right)
        return;
    relay_usb_state_t value = *usb;
    value.revision = r->usb.revision;
    if (r->usb_generation == generation && r->usb.flags == value.flags &&
        r->usb.leds == value.leds && r->usb.protocol == value.protocol &&
        r->usb.rollover == value.rollover && r->usb.vertical == value.vertical &&
        r->usb.horizontal == value.horizontal)
        return;
    bool invalid = generation != r->usb_generation || !(usb->flags & RelayFlag_Configured) ||
                   (usb->flags & RelayFlag_Suspended);
    uint32_t revision = r->usb.revision + 1;
    r->usb_generation = generation;
    r->usb = *usb;
    r->usb.revision = revision;
    if (invalid && (r->active || r->cleanup == Cleanup_Begin))
        leftCleanup(r, Cleanup_Fault, RelayStatus_Unavailable);
    if (r->negotiated)
        stateMessage(r, RelayOp_State);
    changed(r);
}

void Relay_Select(relay_t *r, bool selected)
{
    if (!r->right || r->wanted == selected)
        return;
    r->wanted = selected;
    if (selected)
        r->reason = 0;
    r->wake_sent = false;
    if (!selected)
        rightClose(r, 0);
    changed(r);
}

static void transmitReport(relay_t *r, uint8_t kind, relay_report_slot_t *slot)
{
    relay_message_t m = {
        .op = RelayOp_Report, .token = r->token, .kind = kind, .sequence = slot->sequence};
    memcpy(m.report, slot->report, RelayProtocol_ReportSize(kind));
    sendMessage(r, &m);
    slot->last_tx = r->now;
}

bool Relay_SendReport(relay_t *r, uint8_t kind, const uint8_t *canonical)
{
    if (!r->right || !r->active || !Relay_Awake(r) || !RelayProtocol_ReportSize(kind) || !canonical)
        return false;
    relay_report_slot_t *slot = &r->slots[kind - 1];
    if (slot->pending)
        return false;
    if (slot->sequence == UINT32_MAX) {
        rightClose(r, RelayStatus_ProtocolFault);
        return false;
    }
    ++slot->sequence;
    memcpy(slot->report, canonical, RelayProtocol_ReportSize(kind));
    slot->pending = true;
    slot->started = r->now;
    transmitReport(r, kind, slot);
    return true;
}

static bool peerMatches(const relay_t *r, const relay_message_t *m)
{
    return r->negotiated && m->right_boot == r->peer_boot && m->left_boot == r->boot &&
           m->challenge == r->challenge && m->usb_generation == r->usb_generation;
}

static void receiveLeft(relay_t *r, const relay_message_t *m)
{
    switch (m->op) {
    case RelayOp_Hello:
        if (!m->right_boot || m->schema != USB_LEFT_RELAY_SCHEMA ||
            (m->capabilities & RelayCaps_All) != RelayCaps_All)
            break;
        if (r->peer_boot != m->right_boot) {
            if (r->token)
                leftCleanup(r, Cleanup_End, RelayStatus_Unavailable);
            r->peer_boot = m->right_boot;
            r->challenge = nonce(r);
            r->accepted_selection = 0;
        }
        r->negotiated = true;
        stateMessage(r, RelayOp_HelloAck);
        changed(r);
        return;
    case RelayOp_Begin:
        if (!peerMatches(r, m) || !m->token || !m->selection) {
            result(r, RelayOp_Begun, m->token, 0, 0, RelayStatus_Stale);
            break;
        }
        if (m->selection == r->accepted_selection && m->token == r->token) {
            if (r->active)
                result(r, RelayOp_Begun, m->token, 0, 0, RelayStatus_Ok);
            return;
        }
        if (m->selection <= r->accepted_selection) {
            result(r, RelayOp_Begun, m->token, 0, 0, RelayStatus_Stale);
            break;
        }
        if (r->cleanup || r->active) {
            result(r, RelayOp_Begun, m->token, 0, 0, RelayStatus_Busy);
            return;
        }
        if (!Relay_Awake(r)) {
            result(r, RelayOp_Begun, m->token, 0, 0, RelayStatus_Unavailable);
            return;
        }
        r->token = m->token;
        r->accepted_selection = m->selection;
        memset(r->slots, 0, sizeof(r->slots));
        r->last_rx = r->now;
        r->received_heartbeat = 0;
        leftCleanup(r, Cleanup_Begin, 0);
        return;
    case RelayOp_Report: {
        if (!r->active || m->token != r->token || !Relay_Awake(r))
            break;
        relay_report_slot_t *s = &r->slots[m->kind - 1];
        size_t size = RelayProtocol_ReportSize(m->kind);
        if (m->sequence == s->completed) {
            if (memcmp(m->report, s->completed_report, size)) {
                leftCleanup(r, Cleanup_Fault, RelayStatus_ProtocolFault);
                return;
            }
            ++r->duplicates;
            result(r, RelayOp_Done, r->token, m->kind, m->sequence, RelayStatus_Ok);
            return;
        }
        if (s->pending) {
            if (m->sequence == s->sequence && !memcmp(m->report, s->report, size)) {
                ++r->duplicates;
                return;
            }
            if (m->sequence < s->sequence)
                break;
            leftCleanup(r, Cleanup_Fault, RelayStatus_ProtocolFault);
            return;
        }
        if (s->completed == UINT32_MAX || m->sequence != s->completed + 1)
            break;
        s->pending = true;
        s->submitted = false;
        s->sequence = m->sequence;
        s->started = r->now;
        memcpy(s->report, m->report, size);
        r->last_rx = r->now;
        return;
    }
    case RelayOp_End:
        if (!r->negotiated || !m->token || !m->selection || m->right_boot != r->peer_boot ||
            m->left_boot != r->boot || m->challenge != r->challenge)
            break;
        if (m->token && m->token == r->closed_token) {
            result(r, RelayOp_Ended, m->token, 0, 0, RelayStatus_Ok);
            return;
        }
        if (!r->token && r->quiescent && m->selection > r->accepted_selection) {
            /* END may overtake a lost BEGIN. Fence its selection before proving
             * closure, so a delayed BEGIN can never reopen this transaction. */
            r->accepted_selection = m->selection;
            r->closed_token = m->token;
            result(r, RelayOp_Ended, m->token, 0, 0, RelayStatus_Ok);
            return;
        }
        if (m->token != r->token || m->selection != r->accepted_selection)
            break;
        if (r->cleanup == Cleanup_End)
            return;
        leftCleanup(r, Cleanup_End, 0);
        return;
    case RelayOp_Heartbeat:
        if (!r->active || m->token != r->token || m->sequence <= r->received_heartbeat)
            break;
        r->received_heartbeat = m->sequence;
        r->last_rx = r->now;
        stateMessage(r, RelayOp_State);
        return;
    case RelayOp_Wake:
        if (!peerMatches(r, m))
            break;
        if ((r->usb.flags & RelayFlag_Wakeup) && r->platform.wake)
            r->platform.wake(r->user);
        stateMessage(r, RelayOp_State);
        return;
    default:
        break;
    }
    ++r->rejected;
}

static void receiveRight(relay_t *r, const relay_message_t *m)
{
    switch (m->op) {
    case RelayOp_HelloAck:
        if (m->right_boot != r->boot || !m->left_boot || !m->challenge ||
            m->schema != USB_LEFT_RELAY_SCHEMA ||
            (m->capabilities & RelayCaps_All) != RelayCaps_All)
            break;
        if (r->negotiated && m->left_boot == r->peer_boot && m->challenge == r->challenge &&
            ((int32_t)(m->usb_generation - r->peer_usb_generation) < 0 ||
                (m->usb_generation == r->peer_usb_generation &&
                    (int32_t)(m->state.revision - r->usb.revision) < 0)))
            break;
        if (r->peer_boot != m->left_boot) {
            /* A different boot nonce proves the previous MCU session ended. */
            r->active = r->starting = r->ending = false;
            r->token = 0;
            r->quiescent = true;
            clearPending(r);
        } else if (r->challenge != m->challenge || r->peer_usb_generation != m->usb_generation) {
            rightClose(r, RelayStatus_Unavailable);
        }
        r->peer_boot = m->left_boot;
        r->challenge = m->challenge;
        r->peer_usb_generation = m->usb_generation;
        r->usb = m->state;
        r->negotiated = true;
        r->last_rx = r->now;
        changed(r);
        return;
    case RelayOp_State:
        if (!r->negotiated || m->right_boot != r->boot || m->left_boot != r->peer_boot ||
            m->challenge != r->challenge)
            break;
        if ((int32_t)(m->usb_generation - r->peer_usb_generation) < 0)
            break;
        if (m->usb_generation == r->peer_usb_generation &&
            (int32_t)(m->state.revision - r->usb.revision) < 0)
            break;
        if (m->usb_generation != r->peer_usb_generation) {
            /* SET_PROTOCOL can replace a session without closing endpoints.
                 * Retain the old token until the left proves its transfers drained. */
            rightClose(r, RelayStatus_Unavailable);
        }
        if ((r->usb.flags & RelayFlag_Suspended) != (m->state.flags & RelayFlag_Suspended))
            r->wake_sent = false;
        r->peer_usb_generation = m->usb_generation;
        r->usb = m->state;
        r->last_rx = r->now;
        if (r->active && (!(m->state.flags & RelayFlag_Active) || !Relay_Awake(r)))
            rightClose(r, RelayStatus_Unavailable);
        changed(r);
        return;
    case RelayOp_Begun:
        if (!r->starting || m->token != r->token || m->selection != r->selection ||
            m->usb_generation != r->peer_usb_generation)
            break;
        if (m->status == RelayStatus_Busy)
            return;
        if (m->status != RelayStatus_Ok) {
            rightClose(r, m->status);
            return;
        }
        r->active = true;
        r->starting = false;
        r->reason = 0;
        r->quiescent = false;
        r->last_heartbeat = r->now - RELAY_HEARTBEAT_MS;
        r->heartbeat = 0;
        memset(r->slots, 0, sizeof(r->slots));
        changed(r);
        return;
    case RelayOp_Done: {
        if (!r->active || m->token != r->token)
            break;
        relay_report_slot_t *s = &r->slots[m->kind - 1];
        if (!s->pending || s->sequence != m->sequence) {
            ++r->duplicates;
            return;
        }
        if (m->status != RelayStatus_Ok) {
            rightClose(r, m->status);
            return;
        }
        s->pending = false;
        if (r->platform.delivered)
            r->platform.delivered(r->user, m->kind);
        return;
    }
    case RelayOp_Ended:
        if (!r->ending || m->token != r->token || m->status != RelayStatus_Ok)
            break;
        r->closed_token = r->token;
        r->token = 0;
        r->ending = false;
        r->quiescent = true;
        r->control_started = r->now;
        changed(r);
        return;
    case RelayOp_Fault:
        if (!r->token || m->token != r->token)
            break;
        rightClose(r, m->status);
        return;
    default:
        break;
    }
    ++r->rejected;
}

void Relay_Receive(relay_t *r, const uint8_t *packet, size_t length)
{
    relay_message_t m;
    if (!r->link || !RelayProtocol_Decode(packet, length, &m)) {
        ++r->rejected;
        return;
    }
    ++r->received;
    if (r->right)
        receiveRight(r, &m);
    else
        receiveLeft(r, &m);
}

void Relay_UsbComplete(relay_t *r, uint8_t kind, uint64_t token, uint32_t sequence, bool success)
{
    if (r->right || !RelayProtocol_ReportSize(kind) || token != r->token)
        return;
    relay_report_slot_t *s = &r->slots[kind - 1];
    if (r->cleanup) {
        if (sequence || !s->submitted)
            return;
        s->submitted = false;
        if (success)
            r->neutral_mask |= 1u << (kind - 1);
        return;
    }
    if (!r->active || !s->pending || !s->submitted || sequence != s->sequence)
        return;
    s->submitted = false;
    if (!success) {
        leftCleanup(r, Cleanup_Fault, RelayStatus_TransferFailed);
        return;
    }
    s->completed = sequence;
    memcpy(s->completed_report, s->report, RelayProtocol_ReportSize(kind));
    s->pending = false;
    s->completed_ok = true;
    result(r, RelayOp_Done, token, kind, sequence, RelayStatus_Ok);
}

static void tickLeft(relay_t *r)
{
    if (r->active && elapsed(r->now, r->last_rx, RELAY_LEASE_MS))
        leftCleanup(r, Cleanup_Fault, RelayStatus_LeaseExpired);
    if (r->cleanup) {
        if (!r->quiescent) {
            r->quiescent = r->platform.quiesce(r->user);
            if (!r->quiescent)
                return;
        }
        if (!(r->usb.flags & RelayFlag_Configured))
            r->neutral_mask = 7;
        if (r->usb.flags & RelayFlag_Suspended) {
            /* All user transfers are drained. Retain cleanup so that resume's
             * first output is neutral, while another host can be selected. */
            if (r->cleanup != Cleanup_Begin && r->token) {
                r->closed_token = r->token;
                result(r, RelayOp_Ended, r->token, 0, 0, RelayStatus_Ok);
                r->token = 0;
            }
            return;
        }
        for (uint8_t kind = 1; kind <= RelayKind_Count; ++kind) {
            relay_report_slot_t *s = &r->slots[kind - 1];
            if ((r->neutral_mask & (1u << (kind - 1))) || s->submitted)
                continue;
            memset(s->report, 0, sizeof(s->report));
            s->submitted = true;
            if (r->platform.submit(r->user, kind, s->report, r->token, 0))
                s->submitted = false;
        }
        if (r->neutral_mask == 7) {
            uint8_t cleanup = r->cleanup;
            r->cleanup = Cleanup_None;
            if (cleanup == Cleanup_Begin) {
                r->active = true;
                r->quiescent = false;
                r->last_rx = r->now;
                result(r, RelayOp_Begun, r->token, 0, 0, RelayStatus_Ok);
            } else {
                if (r->token) {
                    r->closed_token = r->token;
                    result(r, RelayOp_Ended, r->token, 0, 0, RelayStatus_Ok);
                    r->token = 0;
                }
                r->quiescent = true;
            }
            ++r->usb.revision;
            stateMessage(r, RelayOp_State);
            changed(r);
        }
        return;
    }
    if (r->active)
        for (uint8_t kind = 1; kind <= RelayKind_Count; ++kind) {
            relay_report_slot_t *s = &r->slots[kind - 1];
            if (!s->pending)
                continue;
            if (elapsed(r->now, s->started, RELAY_REPORT_DEADLINE_MS)) {
                leftCleanup(r, Cleanup_Fault, RelayStatus_TransferFailed);
                return;
            }
            if (s->submitted)
                continue;
            s->submitted = true;
            int ret = r->platform.submit(r->user, kind, s->report, r->token, s->sequence);
            if (ret)
                s->submitted = false;
        }
    if (r->negotiated && elapsed(r->now, r->last_state, 500))
        stateMessage(r, RelayOp_State);
}

static void tickRight(relay_t *r)
{
    if (!r->link) {
        if (r->ending && elapsed(r->now, r->control_started, RELAY_LEASE_MS + 300) && !r->reason) {
            r->reason = RelayStatus_Unavailable;
            ++r->faults;
            changed(r);
        }
        /* Lease expiry closes the left's input gate, but elapsed time alone
         * does not prove DMA or callbacks are drained. Keep the route quarantined. */
        return;
    }
    if (r->negotiated && elapsed(r->now, r->last_rx, 1000)) {
        /* A peer can reboot while UART's ready flag survives. Restart one
         * bounded handshake rather than retry an unknown END token forever. */
        r->negotiated = false;
        r->hello_attempts = 0;
        r->last_hello = r->now;
    }
    if (!r->negotiated) {
        if (r->hello_attempts < 3 && (!r->hello_attempts || elapsed(r->now, r->last_hello, 500))) {
            relay_message_t m = {.op = RelayOp_Hello,
                .right_boot = r->boot,
                .schema = USB_LEFT_RELAY_SCHEMA,
                .capabilities = RelayCaps_All};
            if (sendMessage(r, &m))
                ++r->hello_attempts;
            r->last_hello = r->now;
        }
        if (r->hello_attempts == 3 && elapsed(r->now, r->last_hello, 500) && !r->reason) {
            r->reason = RelayStatus_Incompatible;
            ++r->faults;
            changed(r);
        }
        return;
    }
    if (r->ending) {
        uint32_t interval = RELAY_REPORT_RETRY_MS;
        if (elapsed(r->now, r->control_started, RELAY_REPORT_DEADLINE_MS)) {
            interval = 500;
            if (!r->reason) {
                r->reason = RelayStatus_TransferFailed;
                ++r->faults;
                changed(r);
            }
        }
        if (elapsed(r->now, r->last_control, interval)) {
            relay_message_t m = {.op = RelayOp_End,
                .right_boot = r->boot,
                .left_boot = r->peer_boot,
                .challenge = r->challenge,
                .selection = r->selection,
                .token = r->token};
            sendMessage(r, &m);
            r->last_control = r->now;
        }
        return;
    }
    if (r->starting) {
        if (elapsed(r->now, r->control_started, RELAY_REPORT_DEADLINE_MS)) {
            rightClose(r, RelayStatus_Unavailable);
            return;
        }
        if (elapsed(r->now, r->last_control, RELAY_REPORT_RETRY_MS)) {
            relay_message_t m = {.op = RelayOp_Begin,
                .right_boot = r->boot,
                .left_boot = r->peer_boot,
                .challenge = r->challenge,
                .usb_generation = r->peer_usb_generation,
                .selection = r->selection,
                .token = r->token};
            sendMessage(r, &m);
            r->last_control = r->now;
        }
        return;
    }
    if (!r->active && r->wanted && Relay_Available(r)) {
        if (!Relay_Awake(r)) {
            if (!r->wake_sent && (r->usb.flags & RelayFlag_Wakeup)) {
                relay_message_t m = {.op = RelayOp_Wake,
                    .right_boot = r->boot,
                    .left_boot = r->peer_boot,
                    .challenge = r->challenge,
                    .usb_generation = r->peer_usb_generation,
                    .selection = r->selection};
                if (sendMessage(r, &m))
                    r->wake_sent = true;
            }
            return;
        }
        if (r->reason && !elapsed(r->now, r->control_started, 500))
            return;
        r->starting = true;
        r->token = nonce(r);
        if (++r->selection == 0) {
            r->boot = nonce(r);
            r->negotiated = false;
            r->hello_attempts = 0;
            r->starting = false;
            return;
        }
        r->control_started = r->now;
        r->last_control = r->now - RELAY_REPORT_RETRY_MS;
        return;
    }
    if (r->active) {
        for (uint8_t kind = 1; kind <= RelayKind_Count; ++kind) {
            relay_report_slot_t *s = &r->slots[kind - 1];
            if (!s->pending)
                continue;
            if (elapsed(r->now, s->started, RELAY_REPORT_DEADLINE_MS)) {
                rightClose(r, RelayStatus_TransferFailed);
                return;
            }
            if (elapsed(r->now, s->last_tx, RELAY_REPORT_RETRY_MS)) {
                ++r->retries;
                transmitReport(r, kind, s);
            }
        }
        if (elapsed(r->now, r->last_heartbeat, RELAY_HEARTBEAT_MS)) {
            relay_message_t m = {
                .op = RelayOp_Heartbeat, .token = r->token, .sequence = ++r->heartbeat};
            sendMessage(r, &m);
            r->last_heartbeat = r->now;
        }
        if (elapsed(r->now, r->last_rx, RELAY_LEASE_MS))
            rightClose(r, RelayStatus_Unavailable);
    }
}

void Relay_Tick(relay_t *r, uint32_t now)
{
    r->now = now;
    if (r->right)
        tickRight(r);
    else
        tickLeft(r);
}

#include "host_input_gate.h"
#include "usb_left_relay.h"
#include "usb_left_transfer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct fake fake_t;
struct fake {
    relay_t relay;
    fake_t *peer;
    uint64_t next_nonce;
    uint8_t packets[32][USB_LEFT_RELAY_MAX_PACKET];
    size_t lengths[32], head, tail;
    struct {
        uint64_t token;
        uint32_t seq;
        bool pending;
    } usb[3];
    unsigned submissions[3], delivered[3], drop_done, drop_report, drop_begin, drop_ended;
    bool block_usb, fail_usb, block_drain;
};
static uint64_t randomNonce(void *user)
{
    return ++((fake_t *)user)->next_nonce;
}
static void dirty(void *user)
{
    (void)user;
}
static bool wake(void *user)
{
    (void)user;
    return true;
}
static void delivered(void *user, uint8_t kind)
{
    ++((fake_t *)user)->delivered[kind - 1];
}
static bool sendPacket(void *user, const uint8_t *packet, size_t length)
{
    fake_t *f = user;
    if (packet[1] == RelayOp_Ended && f->drop_ended) {
        --f->drop_ended;
        return true;
    }
    if (packet[1] == RelayOp_Begin && f->drop_begin) {
        --f->drop_begin;
        return true;
    }
    if (packet[1] == RelayOp_Done && f->drop_done) {
        --f->drop_done;
        return true;
    }
    if (packet[1] == RelayOp_Report && f->drop_report) {
        --f->drop_report;
        return true;
    }
    fake_t *p = f->peer;
    assert(p->tail - p->head < 32);
    memcpy(p->packets[p->tail % 32], packet, length);
    p->lengths[p->tail++ % 32] = length;
    return true;
}
static int submit(void *user, uint8_t kind, const uint8_t *report, uint64_t token, uint32_t seq)
{
    fake_t *f = user;
    (void)report;
    if (f->block_usb || f->usb[kind - 1].pending)
        return -1;
    f->usb[kind - 1].pending = true;
    f->usb[kind - 1].token = token;
    f->usb[kind - 1].seq = seq;
    ++f->submissions[kind - 1];
    return 0;
}
static bool quiesce(void *user)
{
    fake_t *f = user;
    if (f->block_drain) {
        return false;
    }
    for (unsigned i = 0; i < 3; ++i)
        f->usb[i].pending = false;
    return true;
}
static void pump(fake_t *f)
{
    while (f->head != f->tail) {
        size_t i = f->head++ % 32;
        Relay_Receive(&f->relay, f->packets[i], f->lengths[i]);
    }
}
static void step(fake_t *a, fake_t *b, uint32_t *now, unsigned ms)
{
    for (unsigned t = 0; t < ms; ++t) {
        Relay_Tick(&a->relay, ++*now);
        Relay_Tick(&b->relay, *now);
        pump(a);
        pump(b);
        for (unsigned i = 0; i < 3; ++i)
            if (b->usb[i].pending && !b->block_usb) {
                b->usb[i].pending = false;
                Relay_UsbComplete(&b->relay, i + 1, b->usb[i].token, b->usb[i].seq, !b->fail_usb);
            }
        pump(a);
        pump(b);
    }
}
static void initPair(fake_t *a, fake_t *b, uint32_t *now)
{
    memset(a, 0, sizeof(*a));
    memset(b, 0, sizeof(*b));
    a->peer = b;
    b->peer = a;
    a->next_nonce = 1000;
    b->next_nonce = 2000;
    relay_platform_t platform = {sendPacket, submit, quiesce, wake, randomNonce, delivered, dirty};
    Relay_Init(&a->relay, true, &platform, a);
    Relay_Init(&b->relay, false, &platform, b);
    relay_usb_state_t state = {.flags = RelayFlag_Configured, .vertical = 1, .horizontal = 1};
    Relay_SetUsb(&b->relay, 1, &state);
    Relay_SetLink(&a->relay, true);
    Relay_SetLink(&b->relay, true);
    *now = 0;
    step(a, b, now, 20);
    assert(a->relay.negotiated && b->relay.negotiated);
    Relay_Select(&a->relay, true);
    step(a, b, now, 20);
    assert(a->relay.active && b->relay.active);
}

static void codecTests(void)
{
    relay_message_t m = {.op = RelayOp_Hello,
        .right_boot = 0x1122334455667788ULL,
        .schema = USB_LEFT_RELAY_SCHEMA,
        .capabilities = RelayCaps_All};
    uint8_t packet[USB_LEFT_RELAY_MAX_PACKET];
    size_t len = RelayProtocol_Encode(&m, packet, sizeof(packet));
    assert(len == 18 && packet[2] == 0x88 && packet[9] == 0x11);
    relay_message_t decoded;
    assert(RelayProtocol_Decode(packet, len, &decoded) && decoded.right_boot == m.right_boot);
    for (size_t size = 0; size < len; ++size)
        assert(!RelayProtocol_Decode(packet, size, &decoded));
    packet[0] = 2;
    assert(!RelayProtocol_Decode(packet, len, &decoded));
    m = (relay_message_t){.op = RelayOp_Report, .token = 7, .kind = 1, .sequence = 1};
    m.report[28] = 0x80;
    assert(!RelayProtocol_Encode(&m, packet, sizeof(packet)));
    m.report[28] = 1;
    assert(RelayProtocol_Encode(&m, packet, sizeof(packet)) == 44);
    assert(RelayProtocol_Decode(packet, 44, &decoded));
    packet[14] = 4;
    assert(!RelayProtocol_Decode(packet, 44, &decoded));
    /* Every short/random frame must be safe to decode, not merely the happy path. */
    for (size_t size = 0; size <= sizeof(packet); ++size) {
        for (unsigned op = 0; op < 256; ++op) {
            memset(packet, 0xff, sizeof(packet));
            packet[0] = 1;
            packet[1] = op;
            RelayProtocol_Decode(packet, size, &decoded);
        }
    }
}

static void lostAckAndDuplicates(void)
{
    fake_t a, b;
    uint32_t now;
    initPair(&a, &b, &now);
    uint8_t report[29] = {0, 1};
    unsigned initial = b.submissions[0];
    b.drop_done = 1;
    assert(Relay_SendReport(&a.relay, 1, report));
    step(&a, &b, &now, 50);
    assert(a.delivered[0] == 1 && b.submissions[0] == initial + 1);
    assert(b.relay.duplicates && a.relay.retries);
    relay_message_t old = {.op = RelayOp_Done, .token = a.relay.token, .kind = 1, .sequence = 1};
    uint8_t packet[64];
    size_t len = RelayProtocol_Encode(&old, packet, sizeof(packet));
    report[1] = 2;
    assert(Relay_SendReport(&a.relay, 1, report));
    Relay_Receive(&a.relay, packet, len);
    assert(a.delivered[0] == 1 && a.relay.slots[0].pending);
    step(&a, &b, &now, 10);
    assert(a.delivered[0] == 2 && b.submissions[0] == initial + 2);
}

static void lostReportAndImmutableMouse(void)
{
    fake_t a, b;
    uint32_t now;
    initPair(&a, &b, &now);
    uint8_t report[29] = {0};
    report[3] = 17;
    a.drop_report = 1;
    unsigned initial = b.submissions[1];
    assert(Relay_SendReport(&a.relay, 2, report));
    report[3] = 99;
    assert(a.relay.slots[1].report[3] == 17);
    step(&a, &b, &now, 50);
    assert(a.delivered[1] == 1 && b.submissions[1] == initial + 1);
    assert(b.relay.slots[1].completed_report[3] == 17);
}

static void staleSessionsAndFailedCompletion(void)
{
    fake_t a, b;
    uint32_t now;
    initPair(&a, &b, &now);
    uint64_t oldToken = a.relay.token;
    Relay_Select(&a.relay, false);
    step(&a, &b, &now, 50);
    assert(a.relay.quiescent && b.relay.quiescent);
    Relay_Select(&a.relay, true);
    step(&a, &b, &now, 50);
    assert(a.relay.active && a.relay.token != oldToken);
    uint8_t report[29] = {0, 1};
    assert(Relay_SendReport(&a.relay, 1, report));
    relay_message_t old = {.op = RelayOp_Done, .token = oldToken, .kind = 1, .sequence = 1};
    uint8_t packet[64];
    size_t len = RelayProtocol_Encode(&old, packet, sizeof(packet));
    Relay_Receive(&a.relay, packet, len);
    assert(a.delivered[0] == 0 && a.relay.slots[0].pending);
    b.fail_usb = true;
    step(&a, &b, &now, 10);
    assert(a.delivered[0] == 0 && !a.relay.active && b.relay.faults);
}

static void leaseAndUsbReset(void)
{
    fake_t a, b;
    uint32_t now;
    initPair(&a, &b, &now);
    for (unsigned i = 0; i < RELAY_LEASE_MS + 10; ++i) {
        Relay_Tick(&b.relay, ++now);
        for (unsigned k = 0; k < 3; ++k)
            if (b.usb[k].pending) {
                b.usb[k].pending = false;
                Relay_UsbComplete(&b.relay, k + 1, b.usb[k].token, b.usb[k].seq, true);
            }
        /* No right loop/heartbeats: discard feedback to avoid filling its FIFO. */
        a.head = a.tail;
    }
    assert(!b.relay.active && b.relay.quiescent && b.relay.reason == RelayStatus_LeaseExpired);
    initPair(&a, &b, &now);
    relay_usb_state_t state = {.flags = RelayFlag_Configured, .vertical = 1, .horizontal = 1};
    Relay_SetUsb(&b.relay, 2, &state);
    step(&a, &b, &now, 30);
    assert(a.relay.peer_usb_generation == 2);
    assert(!a.relay.slots[0].pending);
}

static void inject(relay_t *receiver, const relay_message_t *message)
{
    uint8_t packet[64];
    size_t len = RelayProtocol_Encode(message, packet, sizeof(packet));
    assert(len);
    Relay_Receive(receiver, packet, len);
}
static void controlAndStateGuards(void)
{
    fake_t a, b;
    uint32_t now;
    initPair(&a, &b, &now);
    unsigned before = b.submissions[0];
    relay_message_t begin = {.op = RelayOp_Begin,
        .right_boot = a.relay.boot,
        .left_boot = b.relay.boot,
        .challenge = b.relay.challenge,
        .usb_generation = b.relay.usb_generation,
        .selection = b.relay.accepted_selection,
        .token = b.relay.token};
    inject(&b.relay, &begin);
    pump(&a);
    assert(a.relay.active && b.relay.active && b.submissions[0] == before);
    relay_message_t end = begin;
    end.op = RelayOp_End;
    --end.token;
    inject(&b.relay, &end);
    assert(b.relay.active);
    uint8_t report[29] = {0, 1};
    assert(Relay_SendReport(&a.relay, 1, report));
    step(&a, &b, &now, 10);
    before = b.submissions[0];
    relay_message_t duplicate = {
        .op = RelayOp_Report, .token = b.relay.token, .kind = 1, .sequence = 1};
    duplicate.report[1] =
        2; // Same sequence with different content is a fault, never another USB send.
    inject(&b.relay, &duplicate);
    assert(!b.relay.active && b.submissions[0] == before);
    initPair(&a, &b, &now);
    relay_usb_state_t usb = {.flags = RelayFlag_Configured, .vertical = 256, .horizontal = 256};
    Relay_SetUsb(&b.relay, 2, &usb);
    step(&a, &b, &now, 40);
    relay_message_t stale = {.op = RelayOp_State,
        .right_boot = a.relay.boot,
        .left_boot = b.relay.boot,
        .challenge = b.relay.challenge,
        .usb_generation = 1,
        .state = usb};
    stale.state.revision = UINT32_MAX;
    inject(&a.relay, &stale);
    assert(a.relay.peer_usb_generation == 2);
    stale.op = RelayOp_HelloAck;
    stale.schema = USB_LEFT_RELAY_SCHEMA;
    stale.capabilities = RelayCaps_All;
    inject(&a.relay, &stale);
    assert(a.relay.peer_usb_generation == 2);
}
static void sleepingCleanupAndDeadlines(void)
{
    fake_t a, b;
    uint32_t now;
    initPair(&a, &b, &now);
    uint8_t report[29] = {0, 1};
    assert(Relay_SendReport(&a.relay, 1, report));
    step(&a, &b, &now, 10);
    relay_usb_state_t usb = b.relay.usb;
    usb.flags |= RelayFlag_Suspended;
    Relay_SetUsb(&b.relay, b.relay.usb_generation, &usb);
    Relay_Select(&a.relay, false);
    step(&a, &b, &now, 20);
    assert(!b.relay.active && a.relay.quiescent && b.relay.cleanup);
    unsigned before = b.submissions[0];
    usb.flags &= ~RelayFlag_Suspended;
    Relay_SetUsb(&b.relay, b.relay.usb_generation, &usb);
    step(&a, &b, &now, 20);
    assert(b.relay.quiescent && !b.relay.cleanup && b.submissions[0] == before + 1);
    Relay_Select(&a.relay, true);
    step(&a, &b, &now, 600);
    assert(a.relay.active);
    b.block_usb = true;
    before = b.submissions[0];
    assert(Relay_SendReport(&a.relay, 1, report));
    step(&a, &b, &now, RELAY_REPORT_DEADLINE_MS + 10);
    assert(!a.relay.active && !b.relay.active && a.delivered[0] == 1 && b.submissions[0] == before);
    b.block_usb = false;
    Relay_Select(&a.relay, false);
    step(&a, &b, &now, 550);
    assert(a.relay.quiescent && b.relay.quiescent);
    // Inactive state cannot remain Ready forever if every state response is lost.
    initPair(&a, &b, &now);
    Relay_Select(&a.relay, false);
    step(&a, &b, &now, 20);
    Relay_Tick(&a.relay, now + 1001);
    assert(!Relay_Available(&a.relay));
}
static void resetMustProveQuiescence(void)
{
    fake_t a, b;
    uint32_t now;
    initPair(&a, &b, &now);
    b.block_drain = true;
    relay_usb_state_t usb = b.relay.usb;
    Relay_SetUsb(&b.relay, 2, &usb);
    step(&a, &b, &now, 200);
    assert(!a.relay.active && !a.relay.quiescent && a.relay.ending);
    b.block_drain = false;
    step(&a, &b, &now, 550);
    assert(!a.relay.ending);
    initPair(&a, &b, &now);
    Relay_SetLink(&a.relay, false);
    Relay_Tick(&a.relay, now + 1100);
    assert(
        !a.relay.quiescent && a.relay.ending); // Missing proof cannot become a successful switch.
}
static void lostBeginCanBeClosed(void)
{
    fake_t a, b;
    uint32_t now;
    initPair(&a, &b, &now);
    Relay_Select(&a.relay, false);
    step(&a, &b, &now, 20);
    relay_message_t delayed[2];
    for (unsigned i = 0; i < 2; ++i) {
        a.drop_begin = 1;
        Relay_Select(&a.relay, true);
        step(&a, &b, &now, 2);
        assert(a.relay.starting && !b.relay.token && !a.drop_begin);
        delayed[i] = (relay_message_t){.op = RelayOp_Begin,
            .right_boot = a.relay.boot,
            .left_boot = b.relay.boot,
            .challenge = b.relay.challenge,
            .usb_generation = b.relay.usb_generation,
            .selection = a.relay.selection,
            .token = a.relay.token};
        relay_message_t wrongPeer = delayed[i];
        wrongPeer.op = RelayOp_End;
        wrongPeer.challenge++;
        inject(&b.relay, &wrongPeer);
        assert(b.relay.accepted_selection < a.relay.selection);
        Relay_Select(&a.relay, false);
        step(&a, &b, &now, 20);
        assert(a.relay.quiescent && !a.relay.ending && !b.relay.token);
        assert(b.relay.accepted_selection == delayed[i].selection);
    }
    for (unsigned i = 0; i < 2; ++i) {
        inject(&b.relay, &delayed[i]);
        assert(!b.relay.active && !b.relay.token);
    }
    Relay_Select(&a.relay, true);
    step(&a, &b, &now, 20);
    assert(a.relay.active && b.relay.active);
}

static void sleepingCloseKeepsTombstone(void)
{
    fake_t a, b;
    uint32_t now;
    initPair(&a, &b, &now);
    uint64_t token = a.relay.token;
    relay_usb_state_t usb = b.relay.usb;
    usb.flags |= RelayFlag_Suspended;
    b.drop_ended = 100; // Every close reply during suspension is lost.
    Relay_SetUsb(&b.relay, b.relay.usb_generation, &usb);
    Relay_Select(&a.relay, false);
    step(&a, &b, &now, 20);
    assert(a.relay.ending && b.relay.closed_token == token && !b.relay.token);
    b.drop_ended = 0;
    usb.flags &= ~RelayFlag_Suspended;
    Relay_SetUsb(&b.relay, b.relay.usb_generation, &usb);
    step(&a, &b, &now, 40);
    assert(!b.relay.cleanup && b.relay.closed_token == token && a.relay.quiescent);
}

static void peerRebootWithoutUartDisconnect(void)
{
    fake_t a, b;
    uint32_t now;
    initPair(&a, &b, &now);
    uint64_t previousBoot = b.relay.boot;
    relay_platform_t platform = {sendPacket, submit, quiesce, wake, randomNonce, delivered, dirty};
    memset(b.usb, 0, sizeof(b.usb));
    Relay_Init(&b.relay, false, &platform, &b);
    relay_usb_state_t state = {.flags = RelayFlag_Configured, .vertical = 256, .horizontal = 256};
    Relay_SetUsb(&b.relay, 1, &state);
    Relay_SetLink(&b.relay, true);
    assert(b.relay.boot != previousBoot);
    step(&a, &b, &now, 1200);
    assert(a.relay.negotiated && a.relay.peer_boot == b.relay.boot && a.relay.active &&
           b.relay.active);
}
static void transferGuardsAndHeldSources(void)
{
    uint8_t buffer[32] = {};
    int session = 0, otherSession = 0;
    relay_transfer_ticket_t original = {.session = &session,
        .data = buffer,
        .size = 32,
        .generation = 7,
        .pending = true,
        .usb = true};
    relay_transfer_ticket_t ticket = original, completed;
    assert(RelayTransfer_Complete(&ticket, &otherSession, buffer, 32, 7, &completed) ==
               RelayTransfer_Ignored &&
           ticket.pending);
    assert(RelayTransfer_Complete(&ticket, &session, buffer + 1, 32, 7, &completed) ==
               RelayTransfer_Ignored &&
           ticket.pending);
    assert(RelayTransfer_Complete(&ticket, &session, buffer, 32, 7, &completed) ==
               RelayTransfer_Success &&
           !ticket.pending);
    assert(RelayTransfer_Complete(&ticket, &session, buffer, 32, 7, &completed) ==
           RelayTransfer_Ignored);
    ticket = original;
    assert(RelayTransfer_Complete(&ticket, &session, buffer, 0, 7, &completed) ==
           RelayTransfer_Failed);
    ticket = original;
    assert(RelayTransfer_Complete(&ticket, &session, buffer, 31, 7, &completed) ==
           RelayTransfer_Failed);
    ticket = original;
    assert(RelayTransfer_Complete(&ticket, &session, buffer, 32, 8, &completed) ==
           RelayTransfer_Failed);
    ticket = original;
    ticket.cancelling = true;
    assert(RelayTransfer_Complete(&ticket, &session, buffer, 32, 7, &completed) ==
           RelayTransfer_Failed);
    ticket = original;
    ticket.queued = true;
    ticket.route = 3;
    relay_transfer_ticket_t reservation = ticket;
    assert(RelayTransfer_IsReservation(&ticket, &reservation) && ticket.pending);
    ticket.cancelling = true;
    assert(RelayTransfer_IsReservation(&ticket, &reservation));
    ticket.route++;
    assert(!RelayTransfer_IsReservation(&ticket, &reservation));
    ticket = reservation;
    ticket.generation++;
    assert(!RelayTransfer_IsReservation(&ticket, &reservation));
    ticket = reservation;
    ticket.pending = false;
    assert(!RelayTransfer_IsReservation(&ticket, &reservation));
    uint8_t held[30] = {};
    HostInputGate_Capture(held, 0, true);
    HostInputGate_Capture(held, 239, true);
    HostInputGate_Capture(held, 1, false); // A second physical source can emit the same HID usage.
    assert(HostInputGate_Suppressed(held, 0, false) && !HostInputGate_Suppressed(held, 1, false));
    assert(HostInputGate_Suppressed(held, 239, false));
    assert(!HostInputGate_Suppressed(held, 0, true));
    assert(!HostInputGate_Suppressed(held, 0, false));  // Its next press is fresh.
    assert(HostInputGate_Suppressed(held, 239, false)); // Releasing one source cannot free another.
}

int main(void)
{
    codecTests();
    lostAckAndDuplicates();
    lostReportAndImmutableMouse();
    staleSessionsAndFailedCompletion();
    leaseAndUsbReset();
    controlAndStateGuards();
    sleepingCleanupAndDeadlines();
    transferGuardsAndHeldSources();
    resetMustProveQuiescence();
    peerRebootWithoutUartDisconnect();
    lostBeginCanBeClosed();
    sleepingCloseKeepsTombstone();
    printf("relay tests passed (context %zu bytes)\n", sizeof(relay_t));
}

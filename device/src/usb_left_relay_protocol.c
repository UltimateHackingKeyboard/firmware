#include "usb_left_relay_protocol.h"
#include <string.h>

_Static_assert(USB_LEFT_RELAY_MAX_PACKET + 4 <= 128, "relay exceeds Messenger frame");

size_t RelayProtocol_ReportSize(uint8_t kind)
{
    switch (kind) {
    case RelayKind_Keyboard:
        return USB_LEFT_RELAY_KEYBOARD_SIZE;
    case RelayKind_Mouse:
        return USB_LEFT_RELAY_MOUSE_SIZE;
    case RelayKind_Controls:
        return USB_LEFT_RELAY_CONTROLS_SIZE;
    default:
        return 0;
    }
}

static size_t messageSize(uint8_t op, uint8_t kind)
{
    switch (op) {
    case RelayOp_Hello:
        return 18;
    case RelayOp_HelloAck:
        return 51;
    case RelayOp_State:
        return 43;
    case RelayOp_Begin:
        return 42;
    case RelayOp_Begun:
        return 19;
    case RelayOp_Report:
        return RelayProtocol_ReportSize(kind) ? 15 + RelayProtocol_ReportSize(kind) : 0;
    case RelayOp_Done:
        return 16;
    case RelayOp_End:
        return 38;
    case RelayOp_Ended:
        return 11;
    case RelayOp_Wake:
        return 34;
    case RelayOp_Heartbeat:
        return 14;
    case RelayOp_Fault:
        return 11;
    default:
        return 0;
    }
}

static void put(uint8_t **p, uint64_t value, size_t bytes)
{
    while (bytes--) {
        *(*p)++ = value & 0xff;
        value >>= 8;
    }
}
static uint64_t get(const uint8_t **p, size_t bytes)
{
    uint64_t value = 0;
    for (size_t i = 0; i < bytes; ++i)
        value |= (uint64_t)*(*p)++ << (8 * i);
    return value;
}
static void putState(uint8_t **p, const relay_usb_state_t *s)
{
    put(p, s->revision, 4);
    put(p, s->flags, 2);
    put(p, s->leds, 1);
    put(p, s->protocol, 1);
    put(p, s->rollover, 1);
    put(p, s->vertical, 2);
    put(p, s->horizontal, 2);
}
static void getState(const uint8_t **p, relay_usb_state_t *s)
{
    s->revision = get(p, 4);
    s->flags = get(p, 2);
    s->leds = get(p, 1);
    s->protocol = get(p, 1);
    s->rollover = get(p, 1);
    s->vertical = get(p, 2);
    s->horizontal = get(p, 2);
}

static bool valid(const relay_message_t *m)
{
    if (m->op == RelayOp_Report || m->op == RelayOp_Done) {
        if (!RelayProtocol_ReportSize(m->kind) || m->sequence == 0)
            return false;
    }
    if (m->op == RelayOp_Report && m->kind == RelayKind_Keyboard && (m->report[28] & 0xfc))
        return false;
    if (m->op == RelayOp_HelloAck || m->op == RelayOp_State) {
        if ((m->state.flags & ~RelayFlags_All) || (m->state.leds & ~7) || m->state.protocol > 1 ||
            m->state.rollover > 1 || m->state.vertical == 0 || m->state.horizontal == 0)
            return false;
    }
    if ((m->op == RelayOp_Begun || m->op == RelayOp_Done || m->op == RelayOp_Ended ||
            m->op == RelayOp_Fault) &&
        m->status > RelayStatus_LeaseExpired)
        return false;
    return true;
}

size_t RelayProtocol_Encode(const relay_message_t *m, uint8_t *out, size_t capacity)
{
    if (!m || !out || !valid(m))
        return 0;
    size_t size = messageSize(m->op, m->kind);
    if (!size || capacity < size)
        return 0;
    uint8_t *p = out;
    put(&p, USB_LEFT_RELAY_VERSION, 1);
    put(&p, m->op, 1);
    switch (m->op) {
    case RelayOp_Hello:
        put(&p, m->right_boot, 8);
        put(&p, m->schema, 4);
        put(&p, m->capabilities, 4);
        break;
    case RelayOp_HelloAck:
    case RelayOp_State:
        put(&p, m->right_boot, 8);
        put(&p, m->left_boot, 8);
        put(&p, m->challenge, 8);
        put(&p, m->usb_generation, 4);
        if (m->op == RelayOp_HelloAck) {
            put(&p, m->schema, 4);
            put(&p, m->capabilities, 4);
        }
        putState(&p, &m->state);
        break;
    case RelayOp_Begin:
    case RelayOp_Wake:
        put(&p, m->right_boot, 8);
        put(&p, m->left_boot, 8);
        put(&p, m->challenge, 8);
        put(&p, m->usb_generation, 4);
        put(&p, m->selection, 4);
        if (m->op == RelayOp_Begin)
            put(&p, m->token, 8);
        break;
    case RelayOp_Begun:
        put(&p, m->token, 8);
        put(&p, m->selection, 4);
        put(&p, m->usb_generation, 4);
        put(&p, m->status, 1);
        break;
    case RelayOp_Report:
    case RelayOp_Done:
        put(&p, m->token, 8);
        put(&p, m->sequence, 4);
        put(&p, m->kind, 1);
        if (m->op == RelayOp_Report) {
            memcpy(p, m->report, RelayProtocol_ReportSize(m->kind));
            p += RelayProtocol_ReportSize(m->kind);
        } else
            put(&p, m->status, 1);
        break;
    case RelayOp_End:
        put(&p, m->right_boot, 8);
        put(&p, m->left_boot, 8);
        put(&p, m->challenge, 8);
        put(&p, m->selection, 4);
        put(&p, m->token, 8);
        break;
    case RelayOp_Ended:
    case RelayOp_Fault:
        put(&p, m->token, 8);
        put(&p, m->status, 1);
        break;
    case RelayOp_Heartbeat:
        put(&p, m->token, 8);
        put(&p, m->sequence, 4);
        break;
    default:
        return 0;
    }
    return (size_t)(p - out);
}

bool RelayProtocol_Decode(const uint8_t *data, size_t size, relay_message_t *m)
{
    if (!data || !m || size < 2 || data[0] != USB_LEFT_RELAY_VERSION)
        return false;
    uint8_t op = data[1],
            kind = (op == RelayOp_Report || op == RelayOp_Done) && size >= 15 ? data[14] : 0;
    if (!messageSize(op, kind) || messageSize(op, kind) != size)
        return false;
    memset(m, 0, sizeof(*m));
    m->op = op;
    const uint8_t *p = data + 2;
    switch (op) {
    case RelayOp_Hello:
        m->right_boot = get(&p, 8);
        m->schema = get(&p, 4);
        m->capabilities = get(&p, 4);
        break;
    case RelayOp_HelloAck:
    case RelayOp_State:
        m->right_boot = get(&p, 8);
        m->left_boot = get(&p, 8);
        m->challenge = get(&p, 8);
        m->usb_generation = get(&p, 4);
        if (op == RelayOp_HelloAck) {
            m->schema = get(&p, 4);
            m->capabilities = get(&p, 4);
        }
        getState(&p, &m->state);
        break;
    case RelayOp_Begin:
    case RelayOp_Wake:
        m->right_boot = get(&p, 8);
        m->left_boot = get(&p, 8);
        m->challenge = get(&p, 8);
        m->usb_generation = get(&p, 4);
        m->selection = get(&p, 4);
        if (op == RelayOp_Begin)
            m->token = get(&p, 8);
        break;
    case RelayOp_Begun:
        m->token = get(&p, 8);
        m->selection = get(&p, 4);
        m->usb_generation = get(&p, 4);
        m->status = get(&p, 1);
        break;
    case RelayOp_Report:
    case RelayOp_Done:
        m->token = get(&p, 8);
        m->sequence = get(&p, 4);
        m->kind = get(&p, 1);
        if (op == RelayOp_Report)
            memcpy(m->report, p, RelayProtocol_ReportSize(m->kind));
        else
            m->status = get(&p, 1);
        break;
    case RelayOp_End:
        m->right_boot = get(&p, 8);
        m->left_boot = get(&p, 8);
        m->challenge = get(&p, 8);
        m->selection = get(&p, 4);
        m->token = get(&p, 8);
        break;
    case RelayOp_Ended:
    case RelayOp_Fault:
        m->token = get(&p, 8);
        m->status = get(&p, 1);
        break;
    case RelayOp_Heartbeat:
        m->token = get(&p, 8);
        m->sequence = get(&p, 4);
        break;
    default:
        return false;
    }
    return valid(m);
}

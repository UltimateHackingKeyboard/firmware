#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define USB_LEFT_RELAY_VERSION 1
#define USB_LEFT_RELAY_SCHEMA 0x01DD041Du
#define USB_LEFT_RELAY_MAX_PACKET 64
#define USB_LEFT_RELAY_KEYBOARD_SIZE 29
#define USB_LEFT_RELAY_MOUSE_SIZE 11
#define USB_LEFT_RELAY_CONTROLS_SIZE 10

enum {
    RelayFlag_Configured = 1 << 0,
    RelayFlag_Suspended = 1 << 1,
    RelayFlag_Wakeup = 1 << 2,
    RelayFlag_Windows = 1 << 3,
    RelayFlag_Active = 1 << 4,
    RelayFlag_Fault = 1 << 5,
    RelayFlags_All = 0x3f,
    RelayCap_Keyboard = 1 << 0,
    RelayCap_Mouse = 1 << 1,
    RelayCap_Controls = 1 << 2,
    RelayCap_Wakeup = 1 << 3,
    RelayCap_Feedback = 1 << 4,
    RelayCaps_All = 0x1f,
};

typedef enum {
    RelayOp_Hello = 1,
    RelayOp_HelloAck,
    RelayOp_State,
    RelayOp_Begin,
    RelayOp_Begun,
    RelayOp_Report,
    RelayOp_Done,
    RelayOp_End,
    RelayOp_Ended,
    RelayOp_Wake,
    RelayOp_Heartbeat,
    RelayOp_Fault,
} relay_opcode_t;

typedef enum {
    RelayStatus_Ok,
    RelayStatus_Unavailable,
    RelayStatus_Stale,
    RelayStatus_Incompatible,
    RelayStatus_WakeDenied,
    RelayStatus_TransferFailed,
    RelayStatus_Busy,
    RelayStatus_ProtocolFault,
    RelayStatus_LeaseExpired,
} relay_status_t;

typedef enum {
    RelayKind_Keyboard = 1,
    RelayKind_Mouse,
    RelayKind_Controls,
    RelayKind_Count = 3,
} relay_kind_t;

typedef struct {
    uint32_t revision;
    uint16_t flags;
    uint8_t leds, protocol, rollover;
    uint16_t vertical, horizontal;
} relay_usb_state_t;

/* This is a decoded, host-native value, never a wire struct. */
typedef struct {
    relay_opcode_t op;
    uint64_t right_boot, left_boot, challenge, token;
    uint32_t schema, capabilities, usb_generation, selection, sequence;
    relay_usb_state_t state;
    uint8_t kind, status;
    uint8_t report[USB_LEFT_RELAY_KEYBOARD_SIZE];
} relay_message_t;

size_t RelayProtocol_ReportSize(uint8_t kind);
size_t RelayProtocol_Encode(const relay_message_t *message, uint8_t *out, size_t capacity);
bool RelayProtocol_Decode(const uint8_t *data, size_t size, relay_message_t *message);

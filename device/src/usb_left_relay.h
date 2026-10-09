#pragma once

#include "usb_left_relay_protocol.h"

#define RELAY_REPORT_RETRY_MS 32u
#define RELAY_HEARTBEAT_MS 200u
#define RELAY_LEASE_MS 700u
#define RELAY_REPORT_DEADLINE_MS 128u

typedef struct {
    bool (*send)(void *user, const uint8_t *packet, size_t len);
    int (*submit)(
        void *user, uint8_t kind, const uint8_t *canonical, uint64_t token, uint32_t sequence);
    bool (*quiesce)(void *user);
    bool (*wake)(void *user);
    uint64_t (*nonce)(void *user);
    void (*delivered)(void *user, uint8_t kind);
    void (*changed)(void *user);
} relay_platform_t;

typedef struct {
    uint8_t report[USB_LEFT_RELAY_KEYBOARD_SIZE];
    uint8_t completed_report[USB_LEFT_RELAY_KEYBOARD_SIZE];
    uint32_t sequence, completed, started, last_tx;
    bool pending, submitted, completed_ok;
} relay_report_slot_t;

typedef struct {
    relay_platform_t platform;
    void *user;
    relay_usb_state_t usb;
    relay_report_slot_t slots[RelayKind_Count];
    uint64_t boot, peer_boot, challenge, token, closed_token;
    uint32_t usb_generation, peer_usb_generation, selection, accepted_selection;
    uint32_t now, last_hello, last_state, last_control, last_heartbeat, last_rx;
    uint32_t heartbeat, received_heartbeat, control_started;
    uint32_t received, rejected, duplicates, retries, faults;
    uint8_t hello_attempts, neutral_mask, cleanup, reason;
    bool right, link, negotiated, wanted, active, starting, ending, quiescent, wake_sent;
} relay_t;

void Relay_Init(relay_t *r, bool right, const relay_platform_t *platform, void *user);
void Relay_SetLink(relay_t *r, bool up);
void Relay_SetUsb(relay_t *r, uint32_t generation, const relay_usb_state_t *usb);
void Relay_Tick(relay_t *r, uint32_t now);
void Relay_Receive(relay_t *r, const uint8_t *packet, size_t length);
void Relay_Select(relay_t *r, bool selected);
bool Relay_SendReport(relay_t *r, uint8_t kind, const uint8_t *canonical);
void Relay_UsbComplete(relay_t *r, uint8_t kind, uint64_t token, uint32_t sequence, bool success);
bool Relay_Available(const relay_t *r);
bool Relay_Awake(const relay_t *r);

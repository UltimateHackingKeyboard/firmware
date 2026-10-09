#ifndef __USB_LEFT_RELAY_UHK_H__
#define __USB_LEFT_RELAY_UHK_H__

#include "hid/transport.h"
#include "usb_left_relay.h"

void UsbLeft_Init(void);
bool UsbLeft_Initialized(void);
void UsbLeft_Process(void);
void UsbLeft_Receive(
    uint8_t source, uint8_t connection, uint32_t generation, const uint8_t *packet, size_t len);
uint32_t UsbLeft_IngressGeneration(uint8_t connection);
void UsbLeft_LinkChanged(uint8_t connection);
bool UsbLeft_Connected(void);
bool UsbLeft_Awake(void);
bool UsbLeft_Active(void);
bool UsbLeft_Quiescent(void);
bool UsbLeft_Pending(void);
void UsbLeft_Select(bool selected);
const relay_t *UsbLeft_GetState(void);
int UsbLeft_SendKeyboard(const hid_keyboard_report_t *report);
int UsbLeft_SendMouse(const hid_mouse_report_t *report);
int UsbLeft_SendControls(const hid_controls_report_t *report);
void UsbLeft_LocalComplete(uint8_t kind, uint64_t token, uint32_t sequence, bool success);
void UsbLeft_Provision(void);
int UsbLeft_ProvisionResult(void);
void UsbLeft_RequestControl(uint8_t operation);

/* Local C++ HID adapter. All submission/completion identity is endpoint-local. */
int Hid_LocalUsbSubmit(
    uint8_t kind, const uint8_t *canonical, uint64_t token, uint32_t sequence, uint32_t generation);
bool Hid_LocalUsbQuiesce(void);
void Hid_BeginNeutralization(void);
bool Hid_NeutralizeCurrentHost(void);
bool Hid_NeutralizeLocalUsb(void);
void Hid_LocalUsbSnapshot(uint32_t *generation, relay_usb_state_t *state);
void Hid_LocalUsbSessionChanged(void);
uint32_t Hid_LocalUsbGeneration(void);
bool Hid_LocalUsbCancel(uint8_t kind);
void Hid_LocalUsbService(void);
void Hid_LocalUsbDrainCompletions(void);
bool Hid_LocalUsbQueueSend(uint8_t kind);
void Hid_LocalUsbSendQueued(uint8_t kind);
void Hid_LocalUsbRequestFence(void);
uint8_t Hid_LocalUsbRetireTickets(void);
void Hid_LocalUsbFinishFence(uint32_t generation);
bool Hid_LocalUsbCancellationPending(uint8_t kind, uint32_t *generation);
void Hid_LocalUsbConfigurationChanged(void);
void Hid_LocalUsbComplete(uint8_t kind, const void *session, const void *data, size_t size);
bool Hid_LocalUsbWakeAllowed(void);

/* Coordinator APIs are implemented in host_route.c on the master. */
bool HostRoute_Request(uint8_t host, bool explicitSelection);
void HostRoute_Process(void);
bool HostRoute_Blocked(void);
bool HostRoute_DiscardInput(void);
bool HostRoute_Transitioning(void);
bool HostRoute_Pending(void);
uint8_t HostRoute_Target(void);
uint32_t HostRoute_Generation(void);
void HostRoute_ConfigChanged(void);
bool HostRoute_PrepareConfig(void);
void Connections_CommitHost(uint8_t host, bool explicitSelection);
void UsbReportUpdater_ResetHostInput(bool suppressHeld);

#endif

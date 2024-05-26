#include "messenger.h"
#include "bt_conn.h"
#include "device.h"
#include "keyboard/uart.h"
#include "autoconf.h"
#include "link_protocol.h"
#include "usb/usb_compatibility.h"
#include "nus_server.h"
#include "nus_client.h"
#include "legacy/module.h"
#include "legacy/key_states.h"

static const char* deviceIdToString(device_id_t deviceId) {
    switch (deviceId) {
        case DEVICE_ID_UHK80_LEFT:
            return "left";
        case DEVICE_ID_UHK80_RIGHT:
            return "right";
        case DEVICE_ID_UHK_DONGLE:
            return "dongle";
        default:
            return "unknown";
    }
}

static void sendOverBt(device_id_t dst, message_t message) {
    if (DEVICE_IS_UHK80_LEFT) {
        switch (dst) {
            case DeviceId_Uhk80_Right:
                NusServer_SendMessage(message);
                break;
            default:
                printk("Cannot send message from %s to %s\n", deviceIdToString(DEVICE_ID), deviceIdToString(dst));
        }
    }

    if (DEVICE_IS_UHK80_RIGHT) {
        switch (dst) {
            case DeviceId_Uhk_Dongle:
                NusServer_SendMessage(message);
                break;
            case DeviceId_Uhk80_Left:
                NusClient_SendMessage(message);
                break;
            default:
                printk("Cannot send message from %s to %s\n", deviceIdToString(DEVICE_ID), deviceIdToString(dst));
        }
    }

    if (DEVICE_IS_UHK_DONGLE) {
        switch (dst) {
            case DeviceId_Uhk80_Right:
                NusClient_SendMessage(message);
                break;
            default:
                printk("Cannot send message from %s to %s\n", deviceIdToString(DEVICE_ID), deviceIdToString(dst));
        }
    }
}

static void receiveLeft(device_id_t src, const uint8_t* data, uint16_t len) {
    printk("Didn't expect to receive message %s\n", data);
}

static void receiveRight(device_id_t src, const uint8_t* data, uint16_t len) {
    const uint8_t* message = data+1;
    switch (data[0]) {
        case SyncablePropertyId_LeftHalfKeyStates:
            printk("syncing key states\n");
            for (uint8_t keyId = 0; keyId < MAX_KEY_COUNT_PER_MODULE; keyId++) {
                KeyStates[SlotId_LeftKeyboardHalf][keyId].hardwareSwitchState = !!(message[keyId/8] & (1 << (keyId % 8)));
            }
            break;
        default:
            printk("Unrecognized or unexpected property %i\n", data[0]);
            break;
    }
}

static void receiveDongle(device_id_t src, const uint8_t* data, uint16_t len) {
    const uint8_t* message = data+1;
    switch (data[0]) {
        case SyncablePropertyId_KeyboardReport:
            UsbCompatibility_SendKeyboardReport((usb_basic_keyboard_report_t*)message);
            break;
        case SyncablePropertyId_MouseReport:
            UsbCompatibility_SendMouseReport((usb_mouse_report_t*)message);
            break;
        case SyncablePropertyId_ControlsReport:
            UsbCompatibility_SendConsumerReport2(message);
            break;
        default:
            printk("Unrecognized or unexpected property %i\n", data[0]);
            break;
    }
}

void Messenger_Receive(device_id_t src, const uint8_t* data, uint16_t len) {
    printk("messenger received message from %s\n", deviceIdToString(src));
    switch (DEVICE_ID) {
        case DeviceId_Uhk80_Left:
            receiveLeft(src, data, len);
            break;
        case DeviceId_Uhk80_Right:
            receiveRight(src, data, len);
            break;
        case DeviceId_Uhk_Dongle:
            receiveDongle(src, data, len);
            break;
    }
}

void Messenger_SendMessage(device_id_t dst, message_t message) {
    printk("messenger sending message to %s\n", deviceIdToString(dst));

    if (Uart_IsConnected() && (dst == DeviceId_Uhk80_Left || dst == DeviceId_Uhk80_Right)) {
        printk("  - by uart\n");
        Uart_SendMessage(message);
        return;
    }

    if (Bt_DeviceIsConnected(dst)) {
        printk("  - by nus\n");
        sendOverBt(dst, message);
        return;
    }

    printk("Failed to send message from %s to %s\n", deviceIdToString(DEVICE_ID), deviceIdToString(dst));
}


void Messenger_Send(device_id_t dst, uint8_t messageId, const uint8_t* data, uint16_t len) {
    message_t msg = { .data = data, .len = len, .messageId = messageId };
    Messenger_SendMessage(dst, msg);
}


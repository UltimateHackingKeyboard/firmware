#include "messenger.h"
#include "bt_conn.h"
#include "device.h"
#include "autoconf.h"
#include "link_protocol.h"
#include "messenger_queue.h"
#include "shared/slave_protocol.h"
#include "state_sync.h"
#include "usb/usb_compatibility.h"
#include "nus_server.h"
#include "nus_client.h"
#include "legacy/module.h"
#include "legacy/key_states.h"
#include "shared/attributes.h"
#include "legacy/str_utils.h"
#include "legacy/event_scheduler.h"
#include "legacy/slave_drivers/uhk_module_driver.h"

#if DEVICE_IS_KEYBOARD
#include "keyboard/uart.h"
#endif

static k_tid_t mainThreadId = 0;

typedef enum {
    MessengerChannel_NusServer,
    MessengerChannel_NusClient,
    MessengerChannel_Uart,
    MessengerChannel_None,
} messenger_channel_t;

static messenger_channel_t determineChannel(device_id_t dst) {
#if DEVICE_IS_KEYBOARD
    if (Uart_IsConnected() && (dst == DeviceId_Uhk80_Left || dst == DeviceId_Uhk80_Right)) {
        return MessengerChannel_Uart;
    }
#endif

    if (Bt_DeviceIsConnected(dst)) {
        if (DEVICE_IS_UHK80_LEFT) {
            switch (dst) {
                case DeviceId_Uhk80_Right:
                    return MessengerChannel_NusServer;
                default:
                    return MessengerChannel_None;
            }
        }

        if (DEVICE_IS_UHK80_RIGHT) {
            switch (dst) {
                case DeviceId_Uhk_Dongle:
                    return MessengerChannel_NusServer;
                case DeviceId_Uhk80_Left:
                    return MessengerChannel_NusClient;
                default:
                    return MessengerChannel_None;
            }
        }

        if (DEVICE_IS_UHK_DONGLE) {
            switch (dst) {
                case DeviceId_Uhk80_Right:
                    return MessengerChannel_NusClient;
                default:
                    return MessengerChannel_None;
            }
        }
    }

    return MessengerChannel_None;
}

static void receiveLeft(device_id_t src, const uint8_t* data, uint16_t len) {
    switch (data[0]) {
        case MessageId_StateSync:
            StateSync_ReceiveStateUpdate(src, data, len);
            break;
        default:
            printk("Didn't expect to receive message %i %i\n", data[0], data[1]);
            break;
    }
}

static void processSyncablePropertyRight(device_id_t src, const uint8_t* data, uint16_t len) {

    uint8_t ATTR_UNUSED messageId = *(data++);
    uint8_t propertyId = *(data++);
    const uint8_t* message = data;
    switch (propertyId) {
        case SyncablePropertyId_LeftHalfKeyStates:
            for (uint8_t keyId = 0; keyId < MAX_KEY_COUNT_PER_MODULE; keyId++) {
                KeyStates[SlotId_LeftKeyboardHalf][keyId].hardwareSwitchState = !!(message[keyId/8] & (1 << (keyId % 8)));
            }
            EventVector_Set(EventVector_StateMatrix);
            break;
        case SyncablePropertyId_LeftModuleKeyStates:
            {
                uint8_t driverId = UhkModuleDriverId_LeftModule;
                uhk_module_state_t *moduleState = UhkModuleStates + driverId;
                UhkModuleSlaveDriver_ProcessKeystates(driverId, moduleState, data);
            }
            break;
        default:
            printk("Unrecognized or unexpected message [%i, %i, ...]\n", data[0], data[1]);
            break;
    }
}

static void receiveRight(device_id_t src, const uint8_t* data, uint16_t len) {
    switch (data[0]) {
        case MessageId_StateSync:
            StateSync_ReceiveStateUpdate(src, data, len);
            break;
        case MessageId_SyncableProperty:
            processSyncablePropertyRight(src, data, len);
            break;
        case MessageId_Log:
            printk("%s: %s\n", Utils_DeviceIdToString(src), data);
            break;
        default:
            printk("Unrecognized or unexpected message [%i, %i, ...]\n", data[0], data[1]);
            break;
    }
}

static void processSyncablePropertyDongle(device_id_t src, const uint8_t* data, uint16_t len) {
    uint8_t ATTR_UNUSED messageId = *(data++);
    uint8_t propertyId = *(data++);
    const uint8_t* message = data;
    switch (propertyId) {
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
            printk("Unrecognized or unexpected message [%i, %i, ...]\n", data[0], data[1]);
            break;
    }
}

static void receiveDongle(device_id_t src, const uint8_t* data, uint16_t len) {
    switch (data[0]) {
        case MessageId_SyncableProperty:
            processSyncablePropertyDongle(src, data, len);
            break;
        case MessageId_StateSync:
            StateSync_ReceiveStateUpdate(src, data, len);
            break;
        default:
            printk("Unrecognized or unexpected message [%i, %i, ...]\n", data[0], data[1]);
            break;
    }
}

static void receive(device_id_t src, const uint8_t* data, uint16_t len) {
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

void Messenger_Enqueue(uint8_t src, const uint8_t* data, uint16_t len) {
    if (data[0] != MessageId_Ping) {
        MessengerQueue_Put(src, data, len);
        k_wakeup(mainThreadId);
    }
}

void Messenger_ProcessQueue() {
    messenger_queue_record_t rec = MessengerQueue_Take();
    while (rec.data != NULL) {
        receive(rec.src, rec.data, rec.len);
        MessengerQueue_FreeMemory(rec.data);

        rec = MessengerQueue_Take();
    }
}

bool Messenger_Availability(device_id_t dst, messenger_availability_op_t operation) {
    messenger_channel_t channel = determineChannel(dst);

    switch (channel) {
        case MessengerChannel_Uart:
#if DEVICE_IS_KEYBOARD
            return Uart_Availability(operation);
#endif
            return false;
        case MessengerChannel_NusServer:
            return NusServer_Availability(operation);
        case MessengerChannel_NusClient:
            return NusClient_Availability(operation);
        default:
            return false;
    }
}

void Messenger_SendMessage(device_id_t dst, message_t message) {
    messenger_channel_t channel = determineChannel(dst);

    switch (channel) {
        case MessengerChannel_Uart:
#if DEVICE_IS_KEYBOARD
            Uart_SendMessage(message);
#endif
            break;
        case MessengerChannel_NusServer:
            NusServer_SendMessage(message);
            break;
        case MessengerChannel_NusClient:
            NusClient_SendMessage(message);
            break;
        default:
            printk("Failed to send message from %s to %s\n", Utils_DeviceIdToString(DEVICE_ID), Utils_DeviceIdToString(dst));
            break;
    }
}

void Messenger_Send(device_id_t dst, uint8_t messageId, const uint8_t* data, uint16_t len) {
    message_t msg = { .data = data, .len = len, .messageId[0] = messageId, .idsUsed = 1 };
    Messenger_SendMessage(dst, msg);
}

void Messenger_Send2(device_id_t dst, uint8_t messageId, uint8_t messageId2, const uint8_t* data, uint16_t len) {
    message_t msg = { .data = data, .len = len, .messageId[0] = messageId, .messageId[1] = messageId2, .idsUsed = 2 };
    Messenger_SendMessage(dst, msg);
}

void Messenger_Init() {
    MessengerQueue_Init();
    mainThreadId = k_current_get();
}


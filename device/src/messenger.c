#include "messenger.h"
#include "bt_conn.h"
#include "device.h"
#include "autoconf.h"
#include "link_protocol.h"
#include "logger.h"
#include "main.h"
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
#if DEVICE_IS_UHK80_LEFT
    if (Uart_IsConnected()) {
        switch (dst) {
            case DeviceId_Uhk_Dongle:
            case DeviceId_Uhk80_Right:
                return MessengerChannel_Uart;
            default:
                break;
        }
    }
    if (Bt_DeviceIsConnected(DeviceId_Uhk80_Right)) {
        switch (dst) {
            case DeviceId_Uhk_Dongle:
            case DeviceId_Uhk80_Right:
                return MessengerChannel_NusServer;
            default:
                break;
        }
    }
#endif

#if DEVICE_IS_UHK80_RIGHT
    if (Uart_IsConnected() && (dst == DeviceId_Uhk80_Left)) {
        return MessengerChannel_Uart;
    }
    if (Bt_DeviceIsConnected(dst)) {
        switch (dst) {
            case DeviceId_Uhk_Dongle:
                return MessengerChannel_NusServer;
            case DeviceId_Uhk80_Left:
                return MessengerChannel_NusClient;
            default:
                break;
        }
    }
#endif

#if DEVICE_IS_UHK_DONGLE
    if (Bt_DeviceIsConnected(DeviceId_Uhk80_Right)) {
        switch (dst) {
            case DeviceId_Uhk80_Right:
            case DeviceId_Uhk80_Left:
                return MessengerChannel_NusClient;
            default:
                break;
        }
    }
#endif
    return MessengerChannel_None;
}

static char getDeviceAbbrev(device_id_t src) {
    switch (src) {
        case DeviceId_Uhk80_Left:
            return 'L';
        case DeviceId_Uhk80_Right:
            return 'R';
        case DeviceId_Uhk_Dongle:
            return 'D';
        default:
            return '?';
    }
}

static void receiveLog(device_id_t src, const uint8_t* data, uint16_t len) {
    uint8_t ATTR_UNUSED messageId = *(data++);
    uint8_t logmask = *(data++);
    LogTo(DEVICE_ID, logmask, "%c>>> %s", getDeviceAbbrev(src), data);
}


static void receiveLeft(device_id_t src, const uint8_t* data, uint16_t len) {
    switch (data[0]) {
        case MessageId_StateSync:
            StateSync_ReceiveStateUpdate(src, data, len);
            break;
        case MessageId_Log:
            receiveLog(src, data, len);
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
            receiveLog(src, data, len);
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
        case MessageId_Log:
            receiveLog(src, data, len);
            break;
        default:
            printk("Unrecognized or unexpected message [%i, %i, ...]\n", data[0], data[1]);
            break;
    }
}

static void receive(const uint8_t* data, uint16_t len) {
    device_id_t src = *data++;
    device_id_t dst = *data++;
    len-= 2;

    if (dst != DEVICE_ID) {
        message_t msg = {
            .data = data,
            .len = len,
            .idsUsed = 0,
            .src = src,
            .dst = dst,
        };
        printk("Forwarding message from %d to %d\n", msg.src, msg.dst);
        Messenger_SendMessage(msg);
    } else {
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
}

static bool isSpam(const uint8_t* data) {
    if (data[MessageOffset_MsgId1] == MessageId_Ping) {
        return true;
    }
    if (data[MessageOffset_MsgId1] == MessageId_StateSync && data[MessageOffset_MsgId1+1] == StateSyncPropertyId_Battery) {
        return DEBUG_EVENTLOOP_SCHEDULE;
    }
    return false;
}

ATTR_UNUSED static void getMessageDescription(const uint8_t* data, const char** out1, const char** out2) {
    switch (data[MessageOffset_MsgId1]) {
        case MessageId_StateSync:
            *out1 = "StateSync";
            *out2 = StateSync_PropertyIdToString(data[MessageOffset_MsgId1+1]);
            return;
        case MessageId_SyncableProperty:
            *out1 = "SyncableProperty";
            *out2 = NULL;
            return;
        case MessageId_Log:
            *out1 = "Log";
            *out2 = NULL;
            return;
        case MessageId_Ping:
            *out1 = "Ping";
            *out2 = NULL;
            return;
        default:
            *out1 = "Unknown";
            *out2 = NULL;
            return;
    }
}

void Messenger_Enqueue(uint8_t src, const uint8_t* data, uint16_t len) {
    if (!isSpam(data)) {
        MessengerQueue_Put(src, data, len);
        EventVector_Set(EventVector_NewMessage);
        LOG_SCHEDULE(
            const char* desc1;
            const char* desc2;
            getMessageDescription(data, &desc1, &desc2);
            printk("        (%c %s %s)\n", getDeviceAbbrev(data[MessageOffset_Src]), desc1, desc2 == NULL ? "" : desc2);
        );
        Main_Wake();
    }
}

void Messenger_ProcessQueue() {
    EventVector_Unset(EventVector_NewMessage);
    messenger_queue_record_t rec = MessengerQueue_Take();
    while (rec.data != NULL) {
        receive(rec.data, rec.len);
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
#else
            return false;
#endif
        case MessengerChannel_NusServer:
#ifdef CONFIG_BT_NUS
            return NusServer_Availability(operation);
#else
            return false;
#endif
        case MessengerChannel_NusClient:
#ifdef CONFIG_BT_NUS_CLIENT
            return NusClient_Availability(operation);
#else
            return false;
#endif
        default:
            return false;
    }
}

void Messenger_SendMessage(message_t message) {
    messenger_channel_t channel = determineChannel(message.dst);
    device_id_t dst = message.dst;

    switch (channel) {
        case MessengerChannel_Uart:
#if DEVICE_IS_KEYBOARD
            Uart_SendMessage(message);
#endif
            break;
        case MessengerChannel_NusServer:
#if defined(CONFIG_BT_NUS) && defined(CONFIG_BT_PERIPHERAL)
            NusServer_SendMessage(message);
#endif
            break;
        case MessengerChannel_NusClient:
#ifdef CONFIG_BT_NUS_CLIENT
            NusClient_SendMessage(message);
#endif
            break;
        default:
            printk("Failed to send message from %s to %s\n", Utils_DeviceIdToString(DEVICE_ID), Utils_DeviceIdToString(dst));
            break;
    }
}

void Messenger_Send(device_id_t dst, uint8_t messageId, const uint8_t* data, uint16_t len) {
    message_t msg = {
        .data = data,
        .len = len,
        .messageId[0] = messageId,
        .idsUsed = 1,
        .src = DEVICE_ID,
        .dst = dst,
    };
    Messenger_SendMessage(msg);
}

void Messenger_Send2(device_id_t dst, uint8_t messageId, uint8_t messageId2, const uint8_t* data, uint16_t len) {
    message_t msg = {
        .data = data,
        .len = len,
        .messageId[0] = messageId,
        .messageId[1] = messageId2,
        .idsUsed = 2,
        .src = DEVICE_ID,
        .dst = dst,
    };
    Messenger_SendMessage(msg);
}

void Messenger_Init() {
    MessengerQueue_Init();
    mainThreadId = k_current_get();
}


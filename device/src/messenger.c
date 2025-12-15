#include "messenger.h"
#include "bt_conn.h"
#include "connections.h"
#include "device.h"
#include "autoconf.h"
#include "link_protocol.h"
#include "logger.h"
#include "main.h"
#include "messenger_queue.h"
#include "round_trip_test.h"
#include "shared/slave_protocol.h"
#include "state_sync.h"
#include "thread_stats.h"
#include "usb/usb_compatibility.h"
#include "nus_server.h"
#include "nus_client.h"
#include "module.h"
#include "key_states.h"
#include "shared/attributes.h"
#include "str_utils.h"
#include "event_scheduler.h"
#include "slave_drivers/uhk_module_driver.h"
#include "macros/status_buffer.h"
#include "connections.h"
#include "resend.h"
#include "debug.h"
#include "trace.h"
#include "usb_commands/usb_command_reenumerate.h"
#include "pin_wiring.h"

#if DEVICE_IS_KEYBOARD
#include "keyboard/uart_bridge.h"
#endif

static k_tid_t mainThreadId = 0;

typedef enum {
    MessengerChannel_NusServer,
    MessengerChannel_NusClient,
    MessengerChannel_Uart,
    MessengerChannel_None,
} messenger_channel_t;

static connection_id_t determineChannel(device_id_t dst) {
#if DEVICE_IS_KEYBOARD
    if (DEVICE_IS_UHK80_LEFT && Connections_IsReady(ConnectionId_UartRight)) {
        switch (dst) {
            case DeviceId_Uhk_Dongle:
            case DeviceId_Uhk80_Right:
                return ConnectionId_UartRight;
            default:
                break;
        }
    }

    if (DEVICE_IS_UHK80_RIGHT && Connections_IsReady(ConnectionId_UartLeft)) {
        switch (dst) {
            case DeviceId_Uhk_Dongle:
                break;
            case DeviceId_Uhk80_Left:
                return ConnectionId_UartLeft;
            default:
                break;
        }
    }
#endif

    if (DEVICE_IS_UHK80_LEFT && Connections_IsReady(ConnectionId_NusClientRight)) {
        switch (dst) {
            case DeviceId_Uhk_Dongle:
            case DeviceId_Uhk80_Right:
                return ConnectionId_NusClientRight;
            default:
                return ConnectionId_Invalid;
        }
    }

    if (DEVICE_IS_UHK80_RIGHT) {
        switch (dst) {
            case DeviceId_Uhk_Dongle:
                if (Connections_IsReady(ActiveHostConnectionId) && Connections_Type(ActiveHostConnectionId) == ConnectionType_NusDongle) {
                    return ActiveHostConnectionId;
                }
                break;
            case DeviceId_Uhk80_Left:
                if (Connections_IsReady(ConnectionId_NusServerLeft)) {
                    return ConnectionId_NusServerLeft;
                }
                break;
            default:
                return ConnectionId_Invalid;
        }
    }

    if (DEVICE_IS_UHK_DONGLE) {
        switch (dst) {
            case DeviceId_Uhk80_Right:
            case DeviceId_Uhk80_Left:
                if (Connections_IsReady(ConnectionId_NusServerRight)) {
                    return ConnectionId_NusServerRight;
                }
                break;
            default:
                return ConnectionId_Invalid;
        }
    }

    return ConnectionId_Invalid;
}

uint16_t Messenger_GetMissedMessages(device_id_t dst) {
    connection_id_t connId = determineChannel(dst);
    return Connections[connId].watermarks.missedCount;
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
        case MessageId_RoundTripTest:
            RoundTripTest_Receive(data, len);
            break;
        case MessageId_ResendRequest:
            Resend_ResendRequestReceived(src, determineChannel(src), data, len);
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
        case MessageId_RoundTripTest:
            RoundTripTest_Receive(data, len);
            break;
        case MessageId_ResendRequest:
            Resend_ResendRequestReceived(src, determineChannel(src), data, len);
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
        case MessageId_RoundTripTest:
            RoundTripTest_Receive(data, len);
            break;
        case MessageId_ResendRequest:
            Resend_ResendRequestReceived(src, determineChannel(src), data, len);
            break;
        default:
            printk("Unrecognized or unexpected message [%i, %i, ...]\n", data[0], data[1]);
            break;
    }
}

static void receive(const uint8_t* data, uint16_t len) {
    device_id_t src = data[MessageOffset_Src];
    device_id_t dst = data[MessageOffset_Dst];

    data += MessageOffset_MsgId1;
    len-= MessageOffset_MsgId1;

    if (dst != DEVICE_ID) {
        message_t msg = {
            .data = data,
            .len = len,
            .idsUsed = 0,
            .src = src,
            .dst = dst,
            .connectionId = determineChannel(dst),
        };
        printk("Forwarding message from %d to %d\n", msg.src, msg.dst);
        Messenger_SendMessage(&msg);
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

static bool isSpam(const uint8_t* data, connection_id_t connectionId) {
    if (data[MessageOffset_MsgId1] == MessageId_Ping) {
        return true;
    }
    if (data[MessageOffset_MsgId1] == MessageId_StateSync && data[MessageOffset_MsgId1+1] == StateSyncPropertyId_Battery) {
        return DEBUG_EVENTLOOP_SCHEDULE;
    }
    if (DEVICE_IS_UHK80_RIGHT && Connections_Type(connectionId) == ConnectionType_NusDongle && connectionId != ActiveHostConnectionId) {
        StateSync_UpdateProperty(StateSyncPropertyId_DongleStandby, NULL);
        return true;
    }
    return false;
}

ATTR_UNUSED static void getMessageDescription(uint8_t msgId1, uint8_t msgId2, const char** out1, const char** out2) {

    switch (msgId1) {
        case MessageId_StateSync:
            *out1 = "StateSync";
            *out2 = StateSync_PropertyIdToString(msgId2);
            return;
        case MessageId_SyncableProperty:
            *out1 = "KeyStates";
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
        case MessageId_RoundTripTest:
            *out1 = "RoundTripTest";
            *out2 = NULL;
            return;
        case MessageId_ResendRequest:
            *out1 = "PleaseResend";
            *out2 = NULL;
            return;
        default: {
                static char buffer[3];
                buffer[0] = msgId1+'0';
                buffer[1] = msgId2+'0';
                buffer[2] = '\0';
                *out1 = "Unknown";
                *out2 = buffer;
            }
            return;
    }
}

void Messenger_GetMessageDescription(uint8_t* data, uint8_t offset, const char** out1, const char** out2) {
    getMessageDescription(data[offset+MessageOffset_MsgId1], data[offset+MessageOffset_MsgId1+1], out1, out2);
}

void logAllMessages(uint8_t srcConnectionId, uint8_t src, const uint8_t* data, uint16_t len, uint8_t offset) {
    uint8_t wm = data[offset+MessageOffset_Wm];

    const char *desc1, *desc2;
    uint8_t id1 = data[offset+MessageOffset_MsgId1];
    uint8_t id2 = data[offset+MessageOffset_MsgId1+1];
    getMessageDescription(id1, id2, &desc1, &desc2);
    desc1 = desc1 == NULL ? "" : desc1;
    desc2 = desc2 == NULL ? "" : desc2;

    if (DEBUG_LOG_MESSAGES) {
        LogU("Rec %d    %d %s %s\n", srcConnectionId, wm, desc1, desc2);
    }

    static uint32_t lastTime = 0;
    uint32_t currentTime = k_uptime_get_32();
    uint32_t diff = currentTime - lastTime;
    lastTime = currentTime;
    Trace_Printf("I%d,%d,%d,%d", MessengerQueue_GetOccupiedCount(), id1, id2, diff);
}


bool processWatermarks(uint8_t srcConnectionId, uint8_t src, const uint8_t* data, uint16_t len, uint8_t offset) {
    if (data[offset+MessageOffset_MsgId1] == MessageId_ResendRequest) {
        return true;
    }

    uint8_t wm = data[offset+MessageOffset_Wm];
    uint8_t lastWm = Connections[srcConnectionId].watermarks.rxIdx;
    uint8_t expectedWm = lastWm + 1;

    if (wm == lastWm) {
        // we have already received this message, so don't push it into the queue again.
        return false;
    }

    if (data == MessengerQueue_BlackholeBuffer) {
        return false;
    }

    if (false && wm != expectedWm && DEBUG_MODE) {
        if (wm != 0) {
            int8_t difference = wm - expectedWm;
            LogUSDO("Message index doesn't match by %i message(s) from connection %d (%s), wm %d / %d\n", difference, srcConnectionId, Connections_GetStaticName(srcConnectionId), wm, expectedWm);
        } else {
            // they have resetted their connection; that is fine, just update our watermarks
        }
        Connections[srcConnectionId].watermarks.missedCount++;
    }

    Connections[srcConnectionId].watermarks.rxIdx = wm;

    return true;
}

static void handleCommand(device_id_t src, const uint8_t* data, uint16_t len) {
    uint8_t command = data[MessageOffset_MsgId1+1];
    switch (command) {
        case MessengerCommand_Reboot:
            Reboot(false);
            break;
        default:
            printk("Unknown command: %d\n", command);
            break;
    }
}

void Messenger_Enqueue(uint8_t srcConnectionId, uint8_t src, const uint8_t* data, uint16_t len, uint8_t offset) {
    logAllMessages(srcConnectionId, src, data, len, offset);

    if (data[offset+MessageOffset_MsgId1] == MessageId_Command) {
        handleCommand(src, data+offset, len);
        MessengerQueue_FreeMemory(data);
        return;
    }

    if (!processWatermarks(srcConnectionId, src, data, len, offset)) {
        MessengerQueue_FreeMemory(data);
        return;
    }

    if (isSpam(data+offset, srcConnectionId)) {
        MessengerQueue_FreeMemory(data);
    } else {
        MessengerQueue_Put(src, data, len, offset);
        EventVector_Set(EventVector_NewMessage);
        LOG_SCHEDULE(
            const char* desc1;
            const char* desc2;
            getMessageDescription((data+offset)[0], (data+offset)[1], &desc1, &desc2);
            printk("        (%c %s %s)\n", getDeviceAbbrev(data[MessageOffset_Src]), desc1, desc2 == NULL ? "" : desc2);
        );
        Main_Wake();
    }
}

void Messenger_ProcessQueue() {
    EventVector_Unset(EventVector_NewMessage);
    messenger_queue_record_t rec = MessengerQueue_Take();
    while (rec.data != NULL) {
        Trace('<');
        receive(rec.data+rec.offset, rec.len);
        MessengerQueue_FreeMemory(rec.data);
        Trace('>');

        rec = MessengerQueue_Take();
    }
}

void Messenger_SendMessage(message_t* message) {
    connection_id_t connectionId = message->connectionId;
    device_id_t dst = message->dst;


    switch (connectionId) {
        case ConnectionId_UartLeft:
        case ConnectionId_UartRight:
#if DEVICE_IS_KEYBOARD
            UartBridge_SendMessage(message);
#endif
            break;
        case ConnectionId_NusClientRight:
#if defined(CONFIG_BT_NUS) && defined(CONFIG_BT_PERIPHERAL)
            NusServer_SendMessage(message);
#endif
            break;
        case ConnectionId_HostConnectionFirst ... ConnectionId_HostConnectionLast:
            if (Connections_Type(connectionId) == ConnectionType_NusDongle) {
#if defined(CONFIG_BT_NUS) && defined(CONFIG_BT_PERIPHERAL)
                NusServer_SendMessageTo(message, Peers[Connections[connectionId].peerId].conn);
#endif
            } else {
                printk("Failed to send message from %s to %s; incompatible connection type\n", Utils_DeviceIdToString(DEVICE_ID), Utils_DeviceIdToString(dst));
            }
            break;
        case ConnectionId_NusServerRight:
        case ConnectionId_NusServerLeft:
#ifdef CONFIG_BT_NUS_CLIENT
            NusClient_SendMessage(message);
#endif
            break;
        default:
            printk("Failed to send message from %s to %s\n", Utils_DeviceIdToString(DEVICE_ID), Utils_DeviceIdToString(dst));
            break;
    }

    const char *desc1, *desc2;
    getMessageDescription(message->messageId[0], message->messageId[1], &desc1, &desc2);
    desc1 = desc1 == NULL ? "" : desc1;
    desc2 = desc2 == NULL ? "" : desc2;
    if (DEBUG_LOG_MESSAGES) {
        LogU("Sen %d        %d %s %s\n", connectionId, message->wm, desc1, desc2);
    }

    Trace('O');
}

void Messenger_Send(device_id_t dst, uint8_t messageId, const uint8_t* data, uint16_t len) {
    message_t msg = {
        .data = data,
        .len = len,
        .messageId[0] = messageId,
        .idsUsed = 1,
        .src = DEVICE_ID,
        .dst = dst,
        .connectionId = determineChannel(dst),
    };
    Messenger_SendMessage(&msg);
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
        .connectionId = determineChannel(dst),
    };
    Messenger_SendMessage(&msg);
}

void Messenger_Send2Via(device_id_t dst, connection_id_t connectionId, uint8_t messageId, uint8_t messageId2, const uint8_t* data, uint16_t len) {
    message_t msg = {
        .data = data,
        .len = len,
        .messageId[0] = messageId,
        .messageId[1] = messageId2,
        .idsUsed = 2,
        .src = DEVICE_ID,
        .dst = dst,
        .connectionId = connectionId,
    };
    Messenger_SendMessage(&msg);
}

void Messenger_Init() {
    MessengerQueue_Init();
    mainThreadId = k_current_get();
}


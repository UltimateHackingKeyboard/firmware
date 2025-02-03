#include "resend.h"
#include "connections.h"
#include "device.h"
#include "event_scheduler.h"
#include "link_protocol.h"
#include "messenger.h"
#include "state_sync.h"
#include "keyboard/key_scanner.h"

// TODO: throw out this file and write it anew. This will fail in a number of scenarios.

struct {
    bool needsResend;
    connection_id_t connectionId;
    device_id_t dst;
    uint8_t messageWatermark;
} resendRequest ;

void Resend_RequestResendAsync(device_id_t dst, connection_id_t connectionId, uint8_t messageWatermark) {
    if (!resendRequest.needsResend) {
        resendRequest.needsResend = true;
        resendRequest.connectionId = connectionId;
        resendRequest.messageWatermark = messageWatermark;
        resendRequest.dst = dst;
        EventScheduler_Schedule(CurrentTime, EventSchedulerEvent_ResendMessage, "Messenger_ResendAsync");
    } else {
        // well, we would like to log this, but we don't want to do that from within uart callback
    }

}

void Resend_RequestResendSync() {
    if (resendRequest.needsResend) {
        Messenger_Send2Via(
                resendRequest.dst,
                resendRequest.connectionId,
                MessageId_ResendRequest,
                0,
                &resendRequest.messageWatermark,
                1
                );
        resendRequest.needsResend = false;
    }
}

static void resendSyncableProperty(syncable_property_id_t propId) {
#if DEVICE_IS_KEYBOARD
    switch(propId) {
        case SyncablePropertyId_LeftHalfKeyStates:
            KeyScanner_ResendKeyStates = true;
            break;
        case SyncablePropertyId_LeftModuleKeyStates:
            UhkModuleDriver_ResendKeyStates = true;
            break;
        default:
            LogU("Resend request for %d received. This syncable property is unhandled?.", propId);
    }
#endif
}

void Resend_ResendRequestReceived(device_id_t src, connection_id_t connectionId, const uint8_t* data, uint16_t len) {
    uint8_t ATTR_UNUSED messageId = *(data++);
    uint8_t ATTR_UNUSED dummy = *(data++);
    uint8_t requestedWatermark = *(data++);
    uint8_t lastResendableWatermark = Connections[connectionId].watermarks.lastSentResendableWm;

    if (requestedWatermark != lastResendableWatermark) {
        LogU("Resend request for %d, but we have got %d\n", requestedWatermark, lastResendableWatermark);
        return;
    }

    Connections[connectionId].watermarks.txIdx = Connections[connectionId].watermarks.lastSentResendableWm;
    message_id_t msgId1 = Connections[connectionId].watermarks.lastSentId1;
    message_id_t msgId2 = Connections[connectionId].watermarks.lastSentId2;

    switch (msgId1) {
        case MessageId_StateSync:
            StateSync_UpdateProperty((state_sync_prop_id_t)msgId2, NULL);
            break;
        case MessageId_SyncableProperty:
            resendSyncableProperty((syncable_property_id_t)msgId2);
            break;
        case MessageId_Log:
        case MessageId_RoundTripTest:
        case MessageId_Ping:
            Log("Resend request for %d %d received. Ignoring as it is not vital.", msgId1, msgId2);
            return;
        default:
            Log("Resend request for %d %d received. This message id is unhandled?.", msgId1, msgId2);
            return;
    }
}

void Resend_RegisterMessageAndUpdateWatermarks(message_t* msg) {
    if (msg->messageId[0] != MessageId_ResendRequest) {
        msg->wm = Connections[msg->connectionId].watermarks.txIdx++;
        Connections[msg->connectionId].watermarks.lastSentResendableWm = msg->wm;
        Connections[msg->connectionId].watermarks.lastSentId1 = msg->messageId[0];
        Connections[msg->connectionId].watermarks.lastSentId2 = msg->messageId[1];
    }
}


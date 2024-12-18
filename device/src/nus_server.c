#include "nus_server.h"
#include <bluetooth/services/nus.h>
#include "bt_conn.h"
#include "bt_advertise.h"
#include "bt_conn.h"
#include "messenger.h"
#include "device.h"
#include "messenger_queue.h"
#include "debug.h"
#include "zephyr/bluetooth/addr.h"

#define NUS_SLOTS 2

static K_SEM_DEFINE(nusBusy, NUS_SLOTS, NUS_SLOTS);

static void setPeerConnected(uint8_t peerId, device_id_t peerDeviceId, struct bt_conn *conn) {
    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    if (!Peers[peerId].isConnected || bt_addr_le_eq(&Peers[peerId].addr, addr) != 0) {
        Peers[peerId].addr = *addr;
        Bt_SetDeviceConnected(peerDeviceId);
    }
}

static void received(struct bt_conn *conn, const uint8_t *const data, uint16_t len) {
    uint8_t* copy = MessengerQueue_AllocateMemory();
    memcpy(copy, data, len);

    if (DEVICE_ID == DeviceId_Uhk80_Left) {
        setPeerConnected(PeerIdRight, DeviceId_Uhk80_Right, conn);
    }

    if (DEVICE_ID == DeviceId_Uhk80_Right) {
        setPeerConnected(PeerIdDongle, DeviceId_Uhk_Dongle, conn);
    }

    switch (DEVICE_ID) {
        case DeviceId_Uhk80_Left:
            Messenger_Enqueue(DeviceId_Uhk80_Right, copy, len);
            break;
        case DeviceId_Uhk80_Right:
            Messenger_Enqueue(DeviceId_Uhk_Dongle, copy, len);
            break;
        default:
            printk("Ble received message from unknown source.");
            break;
    }
}

static void sent(struct bt_conn *conn) {
    k_sem_give(&nusBusy);
    char addr[BT_ADDR_LE_STR_LEN] = {0};
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));
}

static void send_enabled(enum bt_nus_send_status status)
{
    if (status == BT_NUS_SEND_STATUS_ENABLED) {
        // in theory, NUS is ready. In practice, it is once we receive a message from the client.
        printk("NUS peripheral connection is ready.\n");
    }
}

static struct bt_nus_cb nus_cb = {
    .received = received,
    .sent = sent,
    .send_enabled = send_enabled,
};

int NusServer_Init(void) {
    int err = bt_nus_init(&nus_cb);
    if (err) {
        printk("Failed to initialize UART service (err: %d)\n", err);
        return err;
    }

    printk("NUS Server module initialized.\n");

    return 0;
}

void NusServer_Disconnected() {
    // I argue that when bt is not connected, freeing the semaphore causes no trouble.
    k_sem_init(&nusBusy, NUS_SLOTS, NUS_SLOTS);
}

static void send_raw_buffer(const uint8_t *data, uint16_t len) {
    SEM_TAKE(&nusBusy);
    int err = bt_nus_send(NULL, data, len);
    if (err) {
        k_sem_give(&nusBusy);
        printk("Failed to send data over BLE connection (err: %d)\n", err);
    }
}

bool NusServer_Availability(messenger_availability_op_t operation) {
    switch (operation) {
        case MessengerAvailabilityOp_InquireOneEmpty:
            return k_sem_count_get(&nusBusy) > 0;
        case MessengerAvailabilityOp_InquireAllEmpty:
            return k_sem_count_get(&nusBusy) == NUS_SLOTS;
        case MessengerAvailabilityOp_BlockTillOneEmpty:
            k_sem_take(&nusBusy, K_FOREVER);
            k_sem_give(&nusBusy);
            return true;
        default:
            return false;
    }
}

void NusServer_SendMessage(message_t msg) {
    uint8_t buffer[MAX_LINK_PACKET_LENGTH];
    uint8_t bufferIdx = 0;

    buffer[bufferIdx++] = msg.src;
    buffer[bufferIdx++] = msg.dst;

    for (uint8_t id = 0; id < msg.idsUsed; id++) {
        buffer[bufferIdx++] = msg.messageId[id];
    }

    if (msg.len + msg.idsUsed + 2 > MAX_LINK_PACKET_LENGTH) {
        printk("Message is too long for NUS packets! [%i, %i, ...]\n", buffer[0], buffer[1]);
        return;
    }

    memcpy(&buffer[bufferIdx], msg.data, msg.len);

    send_raw_buffer(buffer, msg.len+msg.idsUsed+2);
}

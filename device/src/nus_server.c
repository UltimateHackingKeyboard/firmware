#include "nus_server.h"
#include <bluetooth/services/nus.h>
#include "bt_conn.h"
#include "bt_advertise.h"
#include "bt_conn.h"
#include "messenger.h"
#include "device.h"
#include "messenger_queue.h"
#include "legacy/debug.h"

#define NUS_SLOTS 2

static K_SEM_DEFINE(nusBusy, NUS_SLOTS, NUS_SLOTS);

static void received(struct bt_conn *conn, const uint8_t *const data, uint16_t len) {
    uint8_t* copy = MessengerQueue_AllocateMemory();
    memcpy(copy, data, len);

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
        if (DEVICE_ID == DeviceId_Uhk80_Left) {
            Bt_SetDeviceConnected(DeviceId_Uhk80_Right);
        }
        if (DEVICE_ID == DeviceId_Uhk80_Right) {
            Bt_SetDeviceConnected(DeviceId_Uhk_Dongle);
        }
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
    }

    return err;
}

void NusServer_Disconnected() {
    // I argue that when bt is not connected, freeing the semaphore causes no trouble.
    k_sem_init(&nusBusy, NUS_SLOTS, NUS_SLOTS);
}

void NusServer_Send(const uint8_t *data, uint16_t len) {
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

    for (uint8_t id = 0; id < msg.idsUsed; id++) {
        buffer[id] = msg.messageId[id];
    }

    if (msg.len + msg.idsUsed > MAX_LINK_PACKET_LENGTH) {
        printk("Message is too long for NUS packets! [%i, %i, ...]\n", buffer[0], buffer[1]);
        return;
    }

    memcpy(&buffer[msg.idsUsed], msg.data, msg.len);

    NusServer_Send(buffer, msg.len+msg.idsUsed);
}

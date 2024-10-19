#include <bluetooth/services/nus.h>
#include <bluetooth/services/nus_client.h>
#include <bluetooth/scan.h>
#include "bt_scan.h"
#include "bt_conn.h"
#include "device.h"
#include "legacy/usb_interfaces/usb_interface_basic_keyboard.h"
#include "legacy/usb_interfaces/usb_interface_mouse.h"
#include "messenger.h"
#include "nus_client.h"
#include "bool_array_converter.h"
#include "legacy/slot.h"
#include "shared/bool_array_converter.h"
#include "legacy/module.h"
#include "legacy/key_states.h"
#include "keyboard/oled/widgets/console_widget.h"
#include "state_sync.h"
#include "usb/usb_compatibility.h"
#include "link_protocol.h"
#include "messenger_queue.h"
#include "legacy/debug.h"
#include <zephyr/settings/settings.h>

static struct bt_nus_client nus_client;

#define NUS_SLOTS 1
static K_SEM_DEFINE(nusBusy, NUS_SLOTS, NUS_SLOTS);

static void ble_data_sent(struct bt_nus_client *nus, uint8_t err, const uint8_t *const data, uint16_t len) {
    k_sem_give(&nusBusy);
    if (err) {
        printk("ATT error code: 0x%02X\n", err);
    }
}

static uint8_t ble_data_received(struct bt_nus_client *nus, const uint8_t *data, uint16_t len) {
    uint8_t* copy = MessengerQueue_AllocateMemory();
    memcpy(copy, data, len);

    switch (DEVICE_ID) {
        case DeviceId_Uhk80_Right:
            Messenger_Enqueue(DeviceId_Uhk80_Left, copy, len);
            break;
        case DeviceId_Uhk_Dongle:
            Messenger_Enqueue(DeviceId_Uhk80_Right, copy, len);
            break;
        default:
            printk("Ble received message from unknown source.");
            break;
    }
    return BT_GATT_ITER_CONTINUE;
}

static void discovery_complete(struct bt_gatt_dm *dm, void *context) {
    struct bt_nus_client *nus = context;
    int err = 0;

    bt_gatt_dm_data_print(dm);

    err = bt_nus_handles_assign(dm, nus);

    if (err) {
        printk("Could not assign NUS handles (err %d)\n", err);
        return;
    }

    err = bt_nus_subscribe_receive(nus);

    if (err) {
        printk("Could not subscribe to NUS notifications (err %d)\n", err);
        return;
    }

    err = bt_gatt_dm_data_release(dm);

    if (err) {
        printk("Could not release the discovery data (err %d)\n", err);
        return;
    }

    printk("NUS connection with %s successfully established\n", GetPeerStringByConn(nus->conn));

    if (DEVICE_ID == DeviceId_Uhk80_Right) {
        Bt_SetDeviceConnected(DeviceId_Uhk80_Left);
    }
    if (DEVICE_ID == DeviceId_Uhk_Dongle) {
        Bt_SetDeviceConnected(DeviceId_Uhk80_Right);
    }
}

static void discovery_service_not_found(struct bt_conn *conn, void *context) {
    printk("Service not found\n");
}

static void discovery_error(struct bt_conn *conn, int err, void *context) {
    printk("Error while discovering GATT database: (%d)\n", err);
}

struct bt_gatt_dm_cb discovery_cb = {
    .completed         = discovery_complete,
    .service_not_found = discovery_service_not_found,
    .error_found       = discovery_error,
};

static void gatt_discover(struct bt_conn *conn) {
    int err = bt_gatt_dm_start(conn, BT_UUID_NUS_SERVICE, &discovery_cb, &nus_client);
    if (err) {
        printk("could not start the discovery procedure, error code: %d\n", err);
    }
}

static void exchange_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params) {
    if (err) {
        printk("MTU exchange failed with %s (err %u)\n", GetPeerStringByConn(conn), err);
    } else {
        printk("MTU exchange done with %s\n", GetPeerStringByConn(conn));
    }
}

void NusClient_Connect(struct bt_conn *conn) {
    static struct bt_gatt_exchange_params exchange_params;

    exchange_params.func = exchange_func;
    int err = bt_gatt_exchange_mtu(conn, &exchange_params);
    if (err) {
        printk("MTU exchange failed with %s, err %d\n", GetPeerStringByConn(conn), err);
    }

    gatt_discover(conn);

    err = bt_scan_stop();
    if (err && (err != -EALREADY)) {
        printk("Stop LE scan failed (err %d)\n", err);
    }
}

void NusClient_Disconnected() {
    // I argue that when bt is not connected, freeing the semaphore causes no trouble.
    k_sem_init(&nusBusy, NUS_SLOTS, NUS_SLOTS);
}

void NusClient_Init(void) {
    int err;

    struct bt_nus_client_init_param init = {
        .cb = {
            .received = ble_data_received,
            .sent = ble_data_sent,
        }
    };

    err = bt_nus_client_init(&nus_client, &init);
    if (err) {
        printk("NUS Client initialization failed (err %d)\n", err);
        return;
    }

    printk("NUS Client module initialized\n");
}

bool NusClient_Availability(messenger_availability_op_t operation) {
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

static void send_raw_buffer(const uint8_t *data, uint16_t len) {
    SEM_TAKE(&nusBusy);
    int err = bt_nus_client_send(&nus_client, data, len);
    if (err) {
        k_sem_give(&nusBusy);
        printk("Client failed to send data over BLE connection (err %d)\n", err);
    }
}

void NusClient_SendMessage(message_t msg) {
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

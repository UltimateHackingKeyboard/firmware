#ifndef __BT_CONN_H__
#define __BT_CONN_H__

// Includes:

    #include <zephyr/bluetooth/bluetooth.h>
    #include <stdint.h>
    #include "device.h"
    #include "host_connection.h"

// Macros:

    #define PeerNameMaxLength 8

    #define PeerIdUnknown -1
    #define PeerIdLeft 0
    #define PeerIdRight 1
    #define PeerIdHost1 2
    #define PeerIdHost2 3
    #define PeerIdHost3 4
    #define PeerIdFirstHost PeerIdHost1
    #define PeerIdLastHost PeerIdHost3
    #define PeerCount 5


    #define BLE_ADDR_LEN 6
    #define BLE_KEY_LEN 16

// Typedefs:

    typedef enum {
        PeerType_Left,
        PeerType_Right,
        PeerType_Dongle,
        PeerType_BleHid,
    } peer_type_t;

    typedef struct {
        uint8_t id;
        uint8_t connectionId;
        char name[PeerNameMaxLength + 1];
        bt_addr_le_t addr;
        struct bt_conn* conn;
    } peer_t;

// Variables:

    extern peer_t Peers[];
    extern bool Bt_NewPairedDevice;

// Functions:

    int8_t GetPeerIdByConn(const struct bt_conn *conn);
    char *GetPeerStringByAddr(const bt_addr_le_t *addr);
    char *GetPeerStringByConn(const struct bt_conn *conn);
    extern void num_comp_reply(uint8_t accept);

    void BtConn_Init(void);
    void BtConn_DisconnectAll();

    void BtConn_ReviseConnections();
    void Bt_SetConnectionConfigured(struct bt_conn* conn);

#endif // __BT_CONN_H__

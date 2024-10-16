#ifndef __BT_CONN_H__
#define __BT_CONN_H__

// Includes:

    #include <zephyr/bluetooth/bluetooth.h>
    #include <stdint.h>
    #include "device.h"

// Macros:

    #define PeerNameMaxLength 8

    #define PeerIdUnknown -1
    #define PeerIdLeft 0
    #define PeerIdRight 1
    #define PeerIdDongle 2
    #define PeerIdHid 3
    #define PeerCount 4


    #define BLE_ADDR_LEN 6
    #define BLE_KEY_LEN 16

// Typedefs:

    typedef struct {
        uint8_t id;
        char name[PeerNameMaxLength + 1];
        bt_addr_le_t addr;
        bool isConnected;
        bool isConnectedAndConfigured;
    } peer_t;

// Variables:

    extern peer_t Peers[];
    extern bool Bt_NewPairedDevice;

// Functions:

    char *GetPeerStringByAddr(const bt_addr_le_t *addr);
    char *GetPeerStringByConn(const struct bt_conn *conn);
    extern void num_comp_reply(uint8_t accept);

    void Bt_SetDeviceConnected(device_id_t deviceId);
    bool Bt_DeviceIsConnected(uint8_t deviceId);
    void BtConn_Init(void);
    void BtConn_DisconnectAll();

#endif // __BT_CONN_H__

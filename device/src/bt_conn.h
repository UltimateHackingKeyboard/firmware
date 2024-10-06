#ifndef __BT_CONN_H__
#define __BT_CONN_H__

// Includes:

    #include <zephyr/bluetooth/bluetooth.h>
    #include <stdint.h>
    #include "device.h"

// Macros:

    #define PeerCount 3
    #define PeerNameMaxLength 8

    #define PeerIdUnknown -1
    #define PeerIdLeft 0
    #define PeerIdRight 1
    #define PeerIdDongle 2

// Typedefs:

    typedef struct {
        uint8_t id;
        char name[PeerNameMaxLength + 1];
        struct bt_conn *conn;
        bt_addr_le_t addr;
        bool isConnected;
    } peer_t;

// Functions:

    char *GetPeerStringByAddr(const bt_addr_le_t *addr);
    char *GetPeerStringByConn(const struct bt_conn *conn);
    extern void bt_init(void);
    extern void num_comp_reply(uint8_t accept);
    void Bt_UpdatePowerModes();

    void Bt_SetDeviceConnected(device_id_t deviceId);
    bool Bt_DeviceIsConnected(uint8_t deviceId);
// Variables:

    extern peer_t Peers[];

#endif // __BT_CONN_H__

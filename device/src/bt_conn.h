#ifndef __BT_CONN_H__
#define __BT_CONN_H__

// Includes:

    #include <zephyr/bluetooth/bluetooth.h>

// Macros:

    #define PeerCount 3
    #define PeerNameMaxLength 8

// Typedefs:

    typedef struct {
        uint8_t id;
        char name[PeerNameMaxLength + 1];
        bt_addr_le_t addr;
    } peer_t;

// Functions:

    extern void bt_init(void);
    extern void num_comp_reply(uint8_t accept);

// Variables:

    extern peer_t Peers[];

#endif // __BT_CONN_H__

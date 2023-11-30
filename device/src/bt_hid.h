#ifndef __BT_HID_H__
#define __BT_HID_H__

// Includes:

    #include <zephyr/types.h>
    #include <zephyr/bluetooth/conn.h>

// Typedefs:

    typedef struct {
        struct bt_conn *conn;
        bool in_boot_mode;
    } conn_mode_t;

// Variables:

    extern conn_mode_t conn_mode;

// Functions:

    extern void bas_notify(void);
    extern void bluetooth_init(void);
    extern void num_comp_reply(uint8_t accept);
    extern void key_report_send(uint8_t down);
    extern int HidsConnected(struct bt_conn *conn);
    extern int HidsDisconnected(struct bt_conn *conn);

#endif // __BT_HID_H__

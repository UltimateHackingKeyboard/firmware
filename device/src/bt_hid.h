#ifndef __BT_HID_H__
#define __BT_HID_H__

// Includes:

    #include <zephyr/types.h>
    #include <zephyr/bluetooth/conn.h>

// Variables:

    extern struct bt_conn *HidConnection;
    extern bool HidInBootMode;

// Functions:

    extern void bas_notify(void);
    extern void bt_hid_init(void);
    extern void key_report_send(uint8_t down);
    extern int HidsConnected(struct bt_conn *conn);
    extern int HidsDisconnected(struct bt_conn *conn);

#endif // __BT_HID_H__

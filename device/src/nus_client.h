#ifndef __NUS_CLIENT_H__
#define __NUS_CLIENT_H__

// Includes

    #include <zephyr/bluetooth/conn.h>

// Functions:

    extern void NusClient_Init(void);
    extern void gatt_discover(struct bt_conn *conn);
    extern void NusClient_Setup(struct bt_conn *conn);
    extern void NusClient_Send(const uint8_t *data, uint16_t len);

#endif // __NUS_CLIENT_H__

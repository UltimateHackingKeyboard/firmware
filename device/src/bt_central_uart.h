#ifndef __BT_CENTRAL_UART_H__
#define __BT_CENTRAL_UART_H__

// Functions:

    extern void InitCentralUart(void);
    extern void gatt_discover(struct bt_conn *conn);
    extern void SetupCentralConnection(struct bt_conn *conn);
    extern void SendCentralUart(const uint8_t *data, uint16_t len);

#endif // __BT_CENTRAL_UART_H__

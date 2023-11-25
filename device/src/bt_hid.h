#ifndef __BT_HID_H__
#define __BT_HID_H__

// Includes:

    #include <zephyr/types.h>

// Functions:

    extern void bas_notify(void);
    extern void bluetooth_init(void);
    extern void num_comp_reply(uint8_t accept);
    extern void key_report_send(uint8_t down);

#endif // __BT_HID_H__

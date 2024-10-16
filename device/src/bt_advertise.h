#ifndef __BT_ADVERTISE_H__
#define __BT_ADVERTISE_H__

// Includes:

    #include <stdint.h>

// Macros:

    #define ADVERTISE_NUS (1 << 0)
    #define ADVERTISE_HID (1 << 1)

// Functions:

    void BtAdvertise_Start(uint8_t adv_type);
    void BtAdvertise_Stop();
    uint8_t BtAdvertise_Type();

#endif // __BT_ADVERTISE_H__

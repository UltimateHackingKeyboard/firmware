#ifndef __BT_ADVERTISE_H__
#define __BT_ADVERTISE_H__

// Includes:

    #include <stdint.h>

// Macros:

    #define ADVERTISE_NUS (1 << 0)
    #define ADVERTISE_HID (1 << 1)

// Functions:

    extern void Advertise(uint8_t adv_type);

#endif // __BT_ADVERTISE_H__

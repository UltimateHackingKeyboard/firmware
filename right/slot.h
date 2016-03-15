#ifndef __SLOT_H__
#define __SLOT_H__

// Slots provide a way to avoid I2C address collision of modules as only a single module can
// allocate a given slot.

// Macros:

    #define SLOT_ID_RIGHT_KEYBOARD_HALF 0
    #define SLOT_ID_LEFT_KEYBOARD_HALF  1
    #define SLOT_ID_LEFT_MODULE         2
    #define SLOT_ID_RIGHT_MODULE        3
    #define SLOT_ID_BOTH_SIDED_MODULE   4

    // The 7-bit I2C addresses below 0x08 are reserved.
    #define SLOT_I2C_ADDRESS_LEFT_KEYBOARD_HALF 0x08
    #define SLOT_I2C_ADDRESS_LEFT_MODULE        0x09
    #define SLOT_I2C_ADDRESS_RIGHT_MODULE       0x0A
    #define SLOT_I2C_ADDRESS_BOTH_SIDED_MODULE  0x0B
    #define SLOT_I2C_ADDRESS_LEFT_KEYBOARD_HALF 0x0C

    #define SLOT_I2C_ADDRESS_MIN 0x08
    #define SLOT_I2C_ADDRESS_MAX 0x0C

    #define SLOT_COUNT 5

#endif

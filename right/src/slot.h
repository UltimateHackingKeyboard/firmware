#ifndef __SLOT_H__
#define __SLOT_H__

// A slot is a dedicated physical space that can only be occupied by a single module.
// Think of the pogo pin connector of the left keyboard half, for example.
// Given its physical design, it's impossible to mount two modules to a slot.
//
// Slots are useful for two reasons:
// 1. Every slot has a dedicated I2C address. This avoids the I2C address collision of modules.
// 2. There are a limited number of slots available which translates to a maximum number of modules
//    that can be mounted, allowing for the allocation of static memory structures for modules.

// Includes:

#ifdef __ZEPHYR__
    #include "device.h"
#endif

// Macros:

    #define SLOT_COUNT 4
    #define IS_VALID_MODULE_SLOT(slotId) (SlotId_LeftKeyboardHalf <= (slotId) && (slotId) <= SlotId_RightModule)

#ifdef __ZEPHYR__
    #define CURRENT_SLOT_ID (DEVICE_IS_UHK80_LEFT ? SlotId_LeftModule : SlotId_RightModule)
#endif

// Typedefs:

    typedef enum {
        SlotId_RightKeyboardHalf = 0,
        SlotId_LeftKeyboardHalf  = 1,
        SlotId_LeftModule        = 2,
        SlotId_RightModule       = 3,
    } slot_t;

#endif

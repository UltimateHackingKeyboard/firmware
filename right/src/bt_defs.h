#ifndef __BT_DEFS_H__
#define __BT_DEFS_H__

// Includes:
#ifdef __ZEPHYR__
#include "trace.h"
#include <zephyr/kernel.h>
#define BT_TRACE_AND_ASSERT(tag) Trace_Printc(tag); __ASSERT(!k_is_in_isr(), "BLE API called from ISR context!")
#endif

// Macros:

// Typedefs:

    typedef enum {
        PairingMode_Oob = 0,
        PairingMode_PairHid = 1,
        PairingMode_Advertise = 2,
        PairingMode_Default = 3,
        PairingMode_Off = PairingMode_Default,
    } pairing_mode_t;

// Functions:

// Variables

#endif // __BT_PAIR_H__

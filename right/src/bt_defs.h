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
        PairingMode_Oob,
        PairingMode_PairHid,
        PairingMode_Advertise,
        PairingMode_Default,
        PairingMode_Off = PairingMode_Default,
    } pairing_mode_t;

// Functions:

// Variables

#endif // __BT_PAIR_H__

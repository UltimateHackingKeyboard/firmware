#ifndef __LEDMAP_H__
#define __LEDMAP_H__

// Includes:

    #include "key_action.h"

// Typedefs:

    typedef struct {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    } rgb_key_ids_t;

// Variables:

    extern rgb_key_ids_t LedMap[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

#endif

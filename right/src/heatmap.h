#ifndef __HEATMAP_H__
#define __HEATMAP_H__

// Includes:

    #include "key_states.h"

// Typedefs:

// Variables:

extern uint8_t Heat[255];
extern uint16_t HeatMax;

// Functions:

    void Heatmap_SetKeyColors(void);
    void Heatmap_RegisterKeystroke(key_state_t* keystate);

#endif

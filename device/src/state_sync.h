#ifndef __STATE_SYNC_H__
#define __STATE_SYNC_H__

// Includes:

    #include <inttypes.h>
    #include "legacy/layer_switcher.h"

// Typedefs:

// Functions:

    void StateSync_Init();
    void StateSync_UpdateActiveLayer();
    void StateSync_UpdateLayer(layer_id_t layer, bool justClear);
    void StateSync_UpdateBacklight();
    void StateSync_LeftReceiveStateUpdate(const uint8_t* data, uint16_t len);
    void StateSync_RightReceiveStateUpdate(const uint8_t* data, uint16_t len);
    void StateSync_ResetState();

#endif // __STATE_SYNC_H__

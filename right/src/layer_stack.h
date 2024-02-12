#ifndef SRC_LAYER_STACK_H_
#define SRC_LAYER_STACK_H_

// Includes:

    #include "layer.h"

// Macros:

    #define LAYER_STACK_SIZE 10

// Typedefs:
    typedef struct {
        uint8_t layer;
        uint8_t keymap;
        bool held;
        bool removed;
    } layer_stack_record_t;

// Variables:


    extern layer_stack_record_t LayerStack[LAYER_STACK_SIZE];
    extern uint8_t LayerStack_TopIdx;
    extern uint8_t LayerStack_Size;
    extern layer_id_t LayerStack_ActiveLayer;
    extern bool LayerStack_ActiveLayerHeld;

// Functions:

    void LayerStack_Reset();
    void LayerStack_Push(uint8_t layer, uint8_t keymap, bool hold);
    void LayerStack_RemoveRecord(uint8_t stackIndex);
    void LayerStack_Pop(bool forceRemoveTop, bool toggledInsteadOfTop);
    void LayerStack_LegacyPush(layer_id_t layer);
    void LayerStack_LegacyPop(layer_id_t layer);
    uint8_t LayerStack_FindPreviousLayerRecordIdx();
    bool LayerStack_IsLayerToggled();

#endif /* SRC_LAYER_SWITCHER_H_ */

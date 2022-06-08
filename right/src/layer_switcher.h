#ifndef SRC_LAYER_SWITCHER_H_
#define SRC_LAYER_SWITCHER_H_

// Includes:

    #include "fsl_common.h"
    #include "key_states.h"
    #include "layer.h"

// Macros:

// Variables:
    extern uint8_t ActiveLayerModifierMask;
    extern layer_id_t ActiveLayer;
    extern bool ActiveLayerHeld;
    extern uint16_t DoubleTapSwitchLayerTimeout;

// Functions - event triggers:

    void LayerSwitcher_HoldLayer(layer_id_t layer, bool forceSwap);
    void LayerSwitcher_DoubleTapToggle(layer_id_t layer, key_state_t* keyState);
    void LayerSwitcher_DoubleTapInterrupt(key_state_t* keyState);
    void LayerSwitcher_ToggleLayer(layer_id_t layer);
    void LayerSwitcher_UnToggleLayerOnly(layer_id_t layer);

// Functions - hooks:

    void LayerSwitcher_UpdateActiveLayer();

    void LayerSwitcher_RecalculateLayerComposition();

#endif /* SRC_LAYER_SWITCHER_H_ */

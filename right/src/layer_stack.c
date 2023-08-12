#include "layer_stack.h"
#include "keymap.h"
#include "layer.h"
#include "layer_switcher.h"

layer_id_t LayerStack_ActiveLayer = LayerId_Base;
bool LayerStack_ActiveLayerHeld = false;
layer_stack_record_t LayerStack[LAYER_STACK_SIZE];
uint8_t LayerStack_TopIdx;
uint8_t LayerStack_Size;

#define POS(i) ((LayerStack_TopIdx + LAYER_STACK_SIZE + i) % LAYER_STACK_SIZE)

/**
 * This ensures integration/interface between macro layer mechanism
 * and official layer mechanism - we expose our layer via
 * Macros_ActiveLayer/Macros_ActiveLayerHeld and let the layer switcher
 * make its mind.
 */
static void activateLayer(layer_id_t layer)
{
    LayerStack_ActiveLayer = layer;
    LayerStack_ActiveLayerHeld = LayerStack[LayerStack_TopIdx].held;
    if (LayerStack_ActiveLayerHeld) {
        LayerSwitcher_HoldLayer(layer, true);
    } else {
        LayerSwitcher_RecalculateLayerComposition();
    }
}

uint8_t LayerStack_FindPreviousLayerRecordIdx()
{
    for (int i = 1; i < LayerStack_Size; i++) {
        uint8_t pos = POS(-i);
        if (!LayerStack[pos].removed) {
            return pos;
        }
    }
    return LayerStack_TopIdx;
}

static void removeStackTop(bool toggledInsteadOfTop)
{
    if (toggledInsteadOfTop) {
        for (int i = 0; i < LayerStack_Size; i++) {
            uint8_t pos = POS(-i);
            if (!LayerStack[pos].held && !LayerStack[pos].removed) {
                LayerStack[pos].removed = true;
                return;
            }
        }
    } else {
        LayerStack_TopIdx = POS(-1);
        LayerStack_Size--;
    }
}

void LayerStack_RemoveRecord(uint8_t stackIndex)
{
    LayerStack[stackIndex].removed = true;
    LayerStack[stackIndex].held = false;
}

layer_id_t LayerStack_GetTopmostToggledLayer()
{
    for (int i = 0; i < LayerStack_Size; i++) {
        uint8_t pos = POS(-i);
        if (!LayerStack[pos].held && !LayerStack[pos].removed) {
            return LayerStack[pos].layer;
        }
    }
    return LayerId_None;
}

void LayerStack_Pop(bool forceRemoveTop, bool toggledInsteadOfTop)
{
    if (LayerStack_Size > 0 && forceRemoveTop) {
        removeStackTop(toggledInsteadOfTop);
    }
    while(LayerStack_Size > 0 && LayerStack[LayerStack_TopIdx].removed) {
        removeStackTop(false);
    }
    if (LayerStack_Size == 0) {
        LayerStack_Reset();
    }
    if (LayerStack[LayerStack_TopIdx].keymap != CurrentKeymapIndex) {
        SwitchKeymapById(LayerStack[LayerStack_TopIdx].keymap);
    }
    activateLayer(LayerStack[LayerStack_TopIdx].layer);
}

bool LayerStack_IsLegacyToggledContext()
{
    uint8_t firstPos = POS(0);
    uint8_t secondPos = LayerStack_FindPreviousLayerRecordIdx();

    return true
        && LayerStack_Size > 1
        && LayerStack[firstPos].held == false
        && LayerStack[firstPos].removed == false
        && LayerStack[secondPos].held == false
        && LayerStack[secondPos].removed == false
        && LayerStack[secondPos].layer == LayerId_Base
        && LayerStack[secondPos].keymap == CurrentKeymapIndex;
}

// Always maintain protected base layer record. This record cannot be implicit
// due to *KeymapLayer commands.
void LayerStack_Reset()
{
    LayerStack_Size = 1;
    LayerStack[LayerStack_TopIdx].keymap = CurrentKeymapIndex;
    LayerStack[LayerStack_TopIdx].layer = LayerId_Base;
    LayerStack[LayerStack_TopIdx].removed = false;
    LayerStack[LayerStack_TopIdx].held = false;
}

void LayerStack_Push(uint8_t layer, uint8_t keymap, bool hold)
{
    LayerStack_TopIdx = POS(1);
    LayerStack[LayerStack_TopIdx].layer = layer;
    LayerStack[LayerStack_TopIdx].keymap = keymap;
    LayerStack[LayerStack_TopIdx].held = hold;
    LayerStack[LayerStack_TopIdx].removed = false;
    LayerStack_Size = LayerStack_Size < LAYER_STACK_SIZE - 1 ? LayerStack_Size+1 : LayerStack_Size;
    if (keymap != CurrentKeymapIndex) {
        SwitchKeymapById(keymap);
    }
    activateLayer(LayerStack[LayerStack_TopIdx].layer);
}


void LayerStack_LegacyPush(layer_id_t layer)
{
    LayerStack_Push(layer, CurrentKeymapIndex, false);
}

void LayerStack_LegacyPop()
{
    if (LayerStack_IsLegacyToggledContext()) {
        LayerStack_Pop(true, true);
    }
}


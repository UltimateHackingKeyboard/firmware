#include "layer.h"

uint8_t ActiveLayer = LAYER_ID_BASE;

void Layer_MoveTo(uint8_t layer) {
    ActiveLayer = layer;
}

void Layer_MoveToBase() {
    Layer_MoveTo(LAYER_ID_BASE);
}

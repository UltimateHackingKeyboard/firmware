#include "led_display.h"
#include "layer.h"

#define LAYER_LED_FIRST    FRAME_REGISTER_PWM_FIRST + 13
#define LAYER_LED_DISTANCE 16

void LedDisplay_SetLayerLed(uint8_t layerId, uint8_t brightness) {
    for (uint8_t i = 0; i < LAYER_COUNT; i++) {
        LedDriver_WriteRegister(I2C_ADDRESS_LED_DRIVER_LEFT, LAYER_LED_FIRST + (i * LAYER_LED_DISTANCE), brightness * (layerId == i + 1));
    }
}

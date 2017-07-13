#ifndef __LED_DISPLAY_H__
#define __LED_DISPLAY_H__

    #include "peripherals/led_driver.h"

    extern uint8_t LedDisplayBrightness;

    void LedDisplay_SetLayerLed(uint8_t layerId);

#endif

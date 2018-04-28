#ifndef __LED_DISPLAY_H__
#define __LED_DISPLAY_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "layer.h"

// Typedefs:

    typedef enum {
        LedDisplayIcon_CapsLock,
        LedDisplayIcon_Agent,
        LedDisplayIcon_Adaptive,
        LedDisplayIcon_Last = LedDisplayIcon_Adaptive,
    } led_display_icon_t;

// Variables:

    extern uint8_t IconsAndLayerTextsBrightness;
    extern uint8_t AlphanumericSegmentsBrightness;

// Functions:

    void LedDisplay_SetText(uint8_t length, const char* text);
    void LedDisplay_SetCurrentKeymapText(void);
    void LedDisplay_SetLayer(layer_id_t layerId);
    bool LedDisplay_GetIcon(led_display_icon_t icon);
    void LedDisplay_SetIcon(led_display_icon_t icon, bool isEnabled);
    void LedDisplay_UpdateIcons(void);

#endif

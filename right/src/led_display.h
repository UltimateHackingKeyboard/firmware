#ifndef __LED_DISPLAY_H__
#define __LED_DISPLAY_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "layer.h"

// Macros:

    #define LED_DISPLAY_DEBUG_MODE 0
    #define LED_DISPLAY_KEYMAP_NAME_LENGTH 3

// Typedefs:

    typedef enum {
        LedDisplayIcon_CapsLock,
        LedDisplayIcon_Agent,
        LedDisplayIcon_Adaptive,
        LedDisplayIcon_Last = LedDisplayIcon_Adaptive,
        LedDisplayIcon_Count = LedDisplayIcon_Last + 1,
    } led_display_icon_t;

// Variables:

    extern uint32_t LedSleepTimeout;
    extern uint8_t IconsAndLayerTextsBrightness;
    extern uint8_t IconsAndLayerTextsBrightnessDefault;
    extern uint8_t AlphanumericSegmentsBrightness;
    extern uint8_t AlphanumericSegmentsBrightnessDefault;
    extern char LedDisplay_DebugString[];

// Functions:

    void LedDisplay_SetText(uint8_t length, const char* text);
    void LedDisplay_SetLayer(layer_id_t layerId);
    bool LedDisplay_GetIcon(led_display_icon_t icon);
    void LedDisplay_SetIcon(led_display_icon_t icon, bool isEnabled);
    void LedDisplay_UpdateIcons(void);
    void LedDisplay_UpdateText(void);
    void LedDisplay_UpdateAll(void);

#endif

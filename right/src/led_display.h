#ifndef __LED_DISPLAY_H__
#define __LED_DISPLAY_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>

// Typedefs:

    typedef enum {
        LedDisplayIcon_CapsLock,
        LedDisplayIcon_Agent,
        LedDisplayIcon_Adaptive,
    } led_display_icon_t;

// Functions:

    void LedDisplay_SetText(uint8_t length, const char* text);
    void LedDisplay_SetLayer(uint8_t layerId);
    void LedDisplay_SetIcon(led_display_icon_t icon, bool isEnabled);

#endif

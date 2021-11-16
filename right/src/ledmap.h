#ifndef __LEDMAP_H__
#define __LEDMAP_H__

// Includes:

    #include "key_action.h"

// Typedefs:

    typedef enum {
        KeyActionColor_None,
        KeyActionColor_Scancode,
        KeyActionColor_Modifier,
        KeyActionColor_Shortcut,
        KeyActionColor_SwitchLayer,
        KeyActionColor_SwitchKeymap,
        KeyActionColor_Mouse,
        KeyActionColor_Macro,
    } key_action_color_t;

    typedef struct {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    } rgb_t;

// Variables:

    extern rgb_t KeyActionColors[];
    extern rgb_t LedMap[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

// Functions:

    extern void UpdateLayerLeds(void);
    extern void InitLedLayout(void);

#endif

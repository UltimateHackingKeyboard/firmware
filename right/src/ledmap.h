#ifndef __LEDMAP_H__
#define __LEDMAP_H__

// Includes:

    #include "key_action.h"

// Typedefs:

    typedef enum {
        BacklightingMode_Functional,
        BacklightingMode_PerKeyRgb,
        BacklightingMode_ConstantRGB,
    } backlighting_mode_t;

    typedef enum {
        KeyActionColor_None,
        KeyActionColor_Scancode,
        KeyActionColor_Modifier,
        KeyActionColor_Shortcut,
        KeyActionColor_SwitchLayer,
        KeyActionColor_SwitchKeymap,
        KeyActionColor_Mouse,
        KeyActionColor_Macro,
        keyActionColor_Last = KeyActionColor_Macro,
        keyActionColor_Length = keyActionColor_Last + 1,
    } key_action_color_t;

    typedef enum {
        LedMapIndex_LeftSlot_LeftShift = 21,
        LedMapIndex_LeftSlot_IsoKey = 22,
    } led_map_index_t;

// Variables:

    extern backlighting_mode_t BacklightingMode;
    extern rgb_t LedMap_ConstantRGB;
    extern rgb_t KeyActionColors[keyActionColor_Length];

// Functions:

    void UpdateLayerLeds(void);
    void InitLedLayout(void);
    void SetLedBacklightingMode(backlighting_mode_t newMode);

#endif

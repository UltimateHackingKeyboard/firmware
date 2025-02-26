#ifndef __LEDMAP_H__
#define __LEDMAP_H__

// Includes:

    #include "key_action.h"

// Typedefs:

    typedef enum {
        BacklightingMode_Functional,
        BacklightingMode_PerKeyRgb,
        BacklightingMode_ConstantRGB,
        BacklightingMode_Numpad,
        BacklightingMode_LedTest,
        BacklightingMode_LightAll,
        BacklightingMode_Unspecified,
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
        LedMapIndex_LeftSlot_LeftShift = 19,
        LedMapIndex_LeftSlot_IsoKey = 20,
    } led_map_index_t;

    typedef enum {
        ColorMode_Rgb,
        ColorMode_Monochromatic,
    } color_mode_t;

// Variables:

    extern bool Ledmap_LedTestActive;
    extern bool Ledmap_AlwaysOn;

// Functions:

    void Ledmap_UpdateBacklightLeds(void);
    void Ledmap_InitLedLayout(void);
    void Ledmap_SetSfjlValues(void);
    void Ledmap_ActivateTestled(uint8_t slotId, uint8_t keyId);
    void Ledmap_ActivateTestLedMode(bool active);
    void Ledmap_SetLedBacklightingMode(backlighting_mode_t newMode);
    void Ledmap_SetTemporaryLedBacklightingMode(backlighting_mode_t newMode);
    void Ledmap_ResetTemporaryLedBacklightingMode();
    backlighting_mode_t Ledmap_GetEffectiveBacklightMode();

#endif

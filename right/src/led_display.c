#include "led_display.h"
#include "slave_drivers/is31fl3xxx_driver.h"
#include "layer.h"
#include "layer_switcher.h"
#include "keymap.h"
#include "segment_display.h"
#include "led_manager.h"

#ifndef __ZEPHYR__
#include "device.h"
#endif

bool ledIconStates[LedDisplayIcon_Count];

#ifndef __ZEPHYR__
static const uint16_t letterToSegmentMap[] = {


    //DCLMNEGgBKJHFA
    0b00000000000000, // space
    0b00000000000000, // !
    0b00000000001010, // "
    0b00000000000000, // #
    0b11010011001011, // $
    0b00000000000000, // %
    0b00000000000000, // &
    0b00000000010000, // '
    0b00000000000000, // (
    0b00000000000000, // )
    0b00111011011100, // *
    0b00010011001000, // +
    0b00001000000000, // ,
    0b00000011000000, // -
    0b00000000000000, // .
    0b00001000010000, // /
    0b11001100110011, // 0
    0b01000000110000, // 1
    0b10001010100001, // 2
    0b11000011100001, // 3
    0b01000011100010, // 4
    0b10100001000011, // 5
    0b11000111000011, // 6
    0b00010000010001, // 7
    0b11000111100011, // 8
    0b11000011100011, // 9
    0b00000000000000, // :
    0b00000000000000, // ;
    0b00100000010000, // <
    0b10000011000000, // =
    0b00001000000100, // >
    0b00010010100001, // ?
    0b10000110101011, // @
    0b01000111100011, // A
    0b11010010101001, // B
    0b10000100000011, // C
    0b11010000101001, // D
    0b10000111000011, // E
    0b00000111000011, // F
    0b11000110000011, // G
    0b01000111100010, // H
    0b10010000001001, // I
    0b11000000100000, // J
    0b00100101010010, // K
    0b10000100000010, // L
    0b01000100110110, // M
    0b01100100100110, // N
    0b11000100100011, // O
    0b00000111100011, // P
    0b11100100100011, // Q
    0b00100111100011, // R
    0b11000011000011, // S
    0b00010000001001, // T
    0b11000100100010, // U
    0b00001100010010, // V
    0b01101100100010, // W
    0b00101000010100, // X
    0b00010000010100, // Y
    0b10001000010001, // Z
    0b00000000000000, // [
    0b00100000000100, // backslash
    0b00000000000000, // ]
    0b00000000000000, // ^
    0b10000000000000, // _
    0b00000000000100, // `
    0b01000111100011, // a (shown as A) - show lower-case letters as upper-case ones
    0b11010010101001, // b (...)
    0b10000100000011, // c
    0b11010000101001, // d
    0b10000111000011, // e
    0b00000111000011, // f
    0b11000110000011, // g
    0b01000111100010, // h
    0b10010000001001, // i
    0b11000000100000, // j
    0b00100101010010, // k
    0b10000100000010, // l
    0b01000100110110, // m
    0b01100100100110, // n
    0b11000100100011, // o
    0b00000111100011, // p
    0b11100100100011, // q
    0b00100111100011, // r
    0b11000011000011, // s
    0b00010000001001, // t
    0b11000100100010, // u
    0b00001100010010, // v
    0b01101100100010, // w
    0b00101000010100, // x
    0b00010000010100, // y
    0b10001000010001, // z
    0b00100001010000, // {
    0b00010000001000, // |
    0b00001010000100, // }
    0b00010100100000, // ~
};

#define maxSegmentChars 3
#define ledCountPerChar 14

#if DEVICE_ID == DEVICE_ID_UHK60V1

static const uint8_t layerLedIds[LayerId_Count-1] = {0x0d, 0x1d, 0x2d};
static const uint8_t iconLedIds[LedDisplayIcon_Count] = {0x8, 0x09, 0x0a};
static const uint8_t segmentLedIds[maxSegmentChars][ledCountPerChar] = {
    //  A,    F,    H,    J,    K,    B,   G1,   G2,    E,    N,    M,    L,    C     D
    {0x0b, 0x1b, 0x29, 0x2a, 0x2b, 0x0c, 0x1c, 0x28, 0x1a, 0x2c, 0x38, 0x39, 0x18, 0x19},
    {0x3a, 0x4a, 0x58, 0x59, 0x5a, 0x3b, 0x4b, 0x4c, 0x49, 0x5b, 0x5c, 0x68, 0x3c, 0x48},
    {0x69, 0x79, 0x7c, 0x88, 0x89, 0x6a, 0x7a, 0x7b, 0x78, 0x8a, 0x8b, 0x8c, 0x6b, 0x6c},


};

#elif DEVICE_ID == DEVICE_ID_UHK60V2

static const uint8_t layerLedIds[LayerId_Count-1] = {0x99, 0xa9, 0xb9};
static const uint8_t iconLedIds[LedDisplayIcon_Count] = {0x69, 0x79, 0x89};
static const uint8_t segmentLedIds[maxSegmentChars][ledCountPerChar] = {
    //  A,    F,    H,    J,    K,    B,   G1,   G2,    E,    N,    M,    L,    C     D
    {0x60, 0x65, 0x72, 0x73, 0x74, 0x61, 0x70, 0x71, 0x64, 0x75 ,0x78, 0x68, 0x62, 0x63},
    {0x80, 0x85, 0x92, 0x93, 0x94, 0x81, 0x90, 0x91, 0x84, 0x95 ,0x98, 0x88, 0x82, 0x83},
    {0xa0, 0xa5, 0xb2, 0xb3, 0xb4, 0xa1, 0xb0, 0xb1, 0xa4, 0xb5 ,0xb8, 0xa8, 0xa2, 0xa3},

};

#endif

void LedDisplay_SetText(uint8_t length, const char* text)
{
    for (uint8_t charId=0; charId<LED_DISPLAY_KEYMAP_NAME_LENGTH; charId++) {
        char keymapChar = charId < length ? text[charId] : ' ';
        uint16_t charBits = letterToSegmentMap[keymapChar - ' '];
        for (uint8_t ledId=0; ledId<ledCountPerChar; ledId++) {
            uint8_t ledIdx = segmentLedIds[charId][ledId];
            bool isLedOn = charBits & (1 << ledId);

            // Apply segment override if enabled
            if (Uhk60LedOverride.segmentDisplay) {
                uint8_t segmentIdx = charId * ledCountPerChar + ledId;
                isLedOn = Uhk60LedState.segments[segmentIdx] == LED_VALUE_ON;
            }

            LedDriverValues[LedDriverId_Left][ledIdx] = isLedOn ? DisplayBrightness : 0;
        }
    }
}

#endif
void LedDisplay_SetLayer(layer_id_t layerId)
{
#ifndef __ZEPHYR__
    // Layer LED indices in Uhk60LedState.leds: 0=Mod, 1=Fn, 2=Mouse
    bool layerStates[3] = {false, false, false};

    // Determine firmware-intended states
    if (layerId >= 1 && layerId <= 3) {
        layerStates[layerId - 1] = true;
    }
    if (LayerId_Fn2 <= layerId && layerId <= LayerId_Fn5) {
        layerStates[LayerId_Fn - 1] = true;
    }

    // Apply overrides if set
    if (Uhk60LedOverride.mod) {
        layerStates[0] = Uhk60LedState.leds[0] == LED_VALUE_ON;
    }
    if (Uhk60LedOverride.fn) {
        layerStates[1] = Uhk60LedState.leds[1] == LED_VALUE_ON;
    }
    if (Uhk60LedOverride.mouse) {
        layerStates[2] = Uhk60LedState.leds[2] == LED_VALUE_ON;
    }

    // Set the actual LED values
    for (uint8_t i = 0; i < 3; i++) {
        LedDriverValues[LedDriverId_Left][layerLedIds[i]] = layerStates[i] ? DisplayBrightness : 0;
    }
#endif
}

bool LedDisplay_GetIcon(led_display_icon_t icon)
{
#ifndef __ZEPHYR__
    return LedDriverValues[LedDriverId_Left][iconLedIds[icon]];
#else
    return false;
#endif
}

void LedDisplay_SetIcon(led_display_icon_t icon, bool isEnabled)
{
    ledIconStates[icon] = isEnabled;
#ifndef __ZEPHYR__
    bool effectiveState = isEnabled;

    // Check if this icon is overridden and use user-set value instead
    // Icon LED indices in Uhk60LedState.leds: 3=capsLock, 4=agent, 5=adaptive
    switch (icon) {
        case LedDisplayIcon_CapsLock:
            if (Uhk60LedOverride.capsLock) {
                effectiveState = Uhk60LedState.leds[3] == LED_VALUE_ON;
            }
            break;
        case LedDisplayIcon_Agent:
            if (Uhk60LedOverride.agent) {
                effectiveState = Uhk60LedState.leds[4] == LED_VALUE_ON;
            }
            break;
        case LedDisplayIcon_Adaptive:
            if (Uhk60LedOverride.adaptive) {
                effectiveState = Uhk60LedState.leds[5] == LED_VALUE_ON;
            }
            break;
        default:
            break;
    }

    LedDriverValues[LedDriverId_Left][iconLedIds[icon]] = effectiveState ? DisplayBrightness : 0;
#endif
}

void LedDisplay_UpdateIcons(void)
{
    for (led_display_icon_t i=0; i<=LedDisplayIcon_Last; i++) {
        LedDisplay_SetIcon(i, ledIconStates[i]);
    }
}

void LedDisplay_UpdateAll(void)
{
    LedDisplay_UpdateIcons();
    LedDisplay_SetLayer(ActiveLayer);
    SegmentDisplay_UpdateKeymapText();
}

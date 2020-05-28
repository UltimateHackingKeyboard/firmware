#include "led_display.h"
#include "slave_drivers/is31fl37xx_driver.h"
#include "layer.h"
#include "keymap.h"
#include "device.h"

uint8_t IconsAndLayerTextsBrightness = 0xff;
uint8_t AlphanumericSegmentsBrightness = 0xff;
bool ledIconStates[LedDisplayIcon_Count];
char LedDisplay_DebugString[] = "   ";

static const uint16_t letterToSegmentMap[] = {
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
    0b00000000000000, // a
    0b00000000000000, // b
    0b00000000000000, // c
    0b00000000000000, // d
    0b00000000000000, // e
    0b00000000000000, // f
    0b00000000000000, // g
    0b00000000000000, // h
    0b00000000000000, // i
    0b00000000000000, // j
    0b00000000000000, // k
    0b00000000000000, // l
    0b00000000000000, // m
    0b00000000000000, // n
    0b00000000000000, // o
    0b00000000000000, // p
    0b00000000000000, // q
    0b00000000000000, // r
    0b00000000000000, // s
    0b00000000000000, // t
    0b00000000000000, // u
    0b00000000000000, // v
    0b00000000000000, // w
    0b00000000000000, // x
    0b00000000000000, // y
    0b00000000000000, // z
    0b00100001010000, // {
    0b00010000001000, // |
    0b00001010000100, // }
};

#define maxSegmentChars 3
#define ledCountPerChar 14

#if DEVICE_ID == DEVICE_ID_UHK60V1

static const uint8_t layerLedIds[LAYER_COUNT-1] = {13, 29, 45};
static const uint8_t iconLedIds[LedDisplayIcon_Count] = {8, 9, 10};
static const uint8_t segmentLedIds[maxSegmentChars][ledCountPerChar] = {
    {11, 27, 41, 42, 43, 12, 28, 40, 26, 44, 56, 57, 24, 25},
    {58, 74, 88, 89, 90, 59, 75, 76, 73, 91, 92, 104, 60, 72},
    {105, 121, 124, 136, 137, 106, 122, 123, 120, 138, 139, 140, 107, 108},
};

#elif DEVICE_ID == DEVICE_ID_UHK60V2

static const uint8_t layerLedIds[LAYER_COUNT-1] = {153, 169, 185};
static const uint8_t iconLedIds[LedDisplayIcon_Count] = {105, 121, 137};
static const uint8_t segmentLedIds[maxSegmentChars][ledCountPerChar] = {
    {96, 101, 114, 115, 116, 97, 112, 113, 100, 117, 120, 104, 98, 99},
    {128, 133, 146, 147, 148, 129, 144, 145, 132, 149, 152, 136, 130, 131},
    {160, 165, 178, 179, 180, 161, 176, 177, 164, 181, 184, 168, 162, 163},
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
            LedDriverValues[LedDriverId_Left][ledIdx] = isLedOn ? AlphanumericSegmentsBrightness : 0;
        }
    }
}

void LedDisplay_SetLayer(layer_id_t layerId)
{
    for (uint8_t i=1; i<LAYER_COUNT; i++) {
        LedDriverValues[LedDriverId_Left][layerLedIds[i-1]] = layerId == i ? IconsAndLayerTextsBrightness : 0;
    }
}

bool LedDisplay_GetIcon(led_display_icon_t icon)
{
    return LedDriverValues[LedDriverId_Left][iconLedIds[icon]];
}

void LedDisplay_SetIcon(led_display_icon_t icon, bool isEnabled)
{
    ledIconStates[icon] = isEnabled;
    LedDriverValues[LedDriverId_Left][iconLedIds[icon]] = isEnabled ? IconsAndLayerTextsBrightness : 0;
}

void LedDisplay_UpdateIcons(void)
{
    for (led_display_icon_t i=0; i<=LedDisplayIcon_Last; i++) {
        LedDisplay_SetIcon(i, ledIconStates[i]);
    }
}

void LedDisplay_UpdateText(void)
{
#if LED_DISPLAY_DEBUG_MODE == 0
    keymap_reference_t *currentKeymap = AllKeymaps + CurrentKeymapIndex;
    LedDisplay_SetText(currentKeymap->abbreviationLen, currentKeymap->abbreviation);
#else
    LedDisplay_SetText(strlen(LedDisplay_DebugString), LedDisplay_DebugString);
#endif
}

void LedDisplay_UpdateAll(void)
{
    LedDisplay_UpdateIcons();
    LedDisplay_UpdateText();
}

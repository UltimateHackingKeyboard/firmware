#include "led_display.h"
#include "slave_drivers/is31fl3731_driver.h"
#include "layer.h"

static const uint16_t capitalLetterToSegmentSet[] = {
    0b0000000011110111,
    0b0001001010001111,
    0b0000000000111001,
    0b0001001000001111,
    0b0000000011111001,
    0b0000000011110001,
    0b0000000010111101,
    0b0000000011110110,
    0b0001001000001001,
    0b0000000000001110,
    0b0010010001110000,
    0b0000000000111000,
    0b0000010100110110,
    0b0010000100110110,
    0b0000000000111111,
    0b0000000011110011,
    0b0010000000111111,
    0b0010000011110011,
    0b0000000011101101,
    0b0001001000000001,
    0b0000000000111110,
    0b0000110000110000,
    0b0010100000110110,
    0b0010110100000000,
    0b0001010100000000,
    0b0000110000001001,
};

static const uint16_t digitToSegmentSet[] = {
    0b0000110000111111,
    0b0000010000000110,
    0b0000100010001011,
    0b0000000011001111,
    0b0000000011100110,
    0b0010000001101001,
    0b0000000011111101,
    0b0001010000000001,
    0b0000000011111111,
    0b0000000011101111,
};

static uint16_t characterToSegmentSet(char character) {
    switch (character) {
        case 'A' ... 'Z':
            return capitalLetterToSegmentSet[character - 'A'];
        case '0' ... '9':
            return digitToSegmentSet[character - '0'];
    }
    return 0;
}

void LedDisplay_SetText(uint8_t length, const char* text) {
    uint64_t allSegmentSets = 0;

    switch (length) {
        case 3:
            allSegmentSets = (uint64_t)characterToSegmentSet(text[2]) << 28;
        case 2:
            allSegmentSets |= characterToSegmentSet(text[1]) << 14;
        case 1:
            allSegmentSets |= characterToSegmentSet(text[0]);
    }

    LedDriverValues[LedDriverId_Left][11] = allSegmentSets & 0b00000001 ? LED_BRIGHTNESS_LEVEL : 0;
    LedDriverValues[LedDriverId_Left][12] = allSegmentSets & 0b00000010 ? LED_BRIGHTNESS_LEVEL : 0;
    allSegmentSets >>= 2;

    for (uint8_t i = 24; i <= 136; i += 16) {
        for (uint8_t j = 0; j < 5; j++) {
            LedDriverValues[LedDriverId_Left][i + j] = allSegmentSets & 1 << j ? LED_BRIGHTNESS_LEVEL : 0;
        }
        allSegmentSets >>= 5;
    }
}

void LedDisplay_SetLayer(uint8_t layerId) {
    for (uint8_t i = 13; i <= 45; i += 16) {
        LedDriverValues[LedDriverId_Left][i] = 0;
    }

    if (layerId >= LAYER_ID_MOD && layerId <= LAYER_ID_MOUSE) {
        LedDriverValues[LedDriverId_Left][16 * layerId - 3] = LED_BRIGHTNESS_LEVEL;
    }
}

void LedDisplay_SetIcon(led_display_icon_t icon, bool isEnabled) {
    LedDriverValues[LedDriverId_Left][8 + icon] = isEnabled ? LED_BRIGHTNESS_LEVEL : 0;
}

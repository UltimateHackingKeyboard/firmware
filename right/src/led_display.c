#include "led_display.h"
#include "slave_drivers/slave_driver_led_driver.h"
#include "layer.h"

static uint16_t characterToSegmentSet(char character) {
    switch (character) {
    case 'A':
        return 0b0000000011110111;
    case 'B':
        return 0b0001001010001111;
    case 'C':
        return 0b0000000000111001;
    case 'D':
        return 0b0001001000001111;
    case 'E':
        return 0b0000000011111001;
    case 'F':
        return 0b0000000011110001;
    case 'G':
        return 0b0000000010111101;
    case 'H':
        return 0b0000000011110110;
    case 'I':
        return 0b0001001000001001;
    case 'J':
        return 0b0000000000001110;
    case 'K':
        return 0b0011011000000000;
    case 'L':
        return 0b0000000000111000;
    case 'M':
        return 0b0000010100110110;
    case 'N':
        return 0b0010000100110110;
    case 'O':
        return 0b0000000000111111;
    case 'P':
        return 0b0000000011110011;
    case 'Q':
        return 0b0010000000111111;
    case 'R':
        return 0b0010000011110011;
    case 'S':
        return 0b0000000011101101;
    case 'T':
        return 0b0001001000000001;
    case 'U':
        return 0b0000000000111110;
    case 'V':
        return 0b0000110000110000;
    case 'W':
        return 0b0010100000110110;
    case 'X':
        return 0b0010110100000000;
    case 'Y':
        return 0b0001010100000000;
    case 'Z':
        return 0b0000110000001001;
    case '0':
        return 0b0000110000111111;
    case '1':
        return 0b0000010000000110;
    case '2':
        return 0b0000100010001011;
    case '3':
        return 0b0000000011001111;
    case '4':
        return 0b0000000011100110;
    case '5':
        return 0b0010000001101001;
    case '6':
        return 0b0000000011110111;
    case '7':
        return 0b0000000000000111;
    case '8':
        return 0b0000000011111111;
    case '9':
        return 0b0000000011101111;
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
    LedDriverStates[LedDriverId_Left].ledValues[11] = allSegmentSets & 0b00000001 ? 255 : 0;
    LedDriverStates[LedDriverId_Left].ledValues[12] = allSegmentSets & 0b00000010 ? 255 : 0;
    allSegmentSets >>= 2;
    for (uint8_t i = 24; i <= 136; i += 16) {
        for (uint8_t j = 0; j < 5; j++) {
            LedDriverStates[LedDriverId_Left].ledValues[i + j] = allSegmentSets & 1 << j ? 255 : 0;
        }
        allSegmentSets >>= 5;
    }
}


void LedDisplay_SetLayer(uint8_t layerId) {
    uint8_t layerLedValues[3] = { 0 };

    if (layerId >= LAYER_ID_MOD && layerId <= LAYER_ID_MOUSE) {
        layerLedValues[layerId - 1] = 0xFF;
    }
    LedDriverStates[LedDriverId_Left].ledValues[13] = layerLedValues[0];
    LedDriverStates[LedDriverId_Left].ledValues[29] = layerLedValues[1];
    LedDriverStates[LedDriverId_Left].ledValues[45] = layerLedValues[2];
}

void LedDisplay_SetIcon(led_display_icon_t icon, bool isEnabled) {
    LedDriverStates[LedDriverId_Left].ledValues[8 + icon] = isEnabled ? 255 : 0;
}

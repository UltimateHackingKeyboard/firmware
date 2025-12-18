#include "heatmap.h"
#include "attributes.h"
#include "utils.h"
#include "ledmap.h"
#include "event_scheduler.h"
#include "key_states.h"
#include "keymap.h"
#include "device.h"

#ifdef __ZEPHYR__
#include "state_sync.h"
#endif

#define RETAIN 255
#define KEYCOUNT 30

uint8_t Heat[255] = { 0 };
uint16_t HeatMax = 0;
uint16_t HeatSum = 0;

static void renormalize(void) {
    HeatMax = HeatMax*2/3;
    HeatSum = 0;
    for (uint8_t i = 0; i < 255; i++) {
        Heat[i] = Heat[i]*2/3;
        HeatSum += Heat[i];
    }
}

void Heatmap_RegisterKeystroke(key_state_t* keystate) {
    if (Ledmap_GetEffectiveBacklightMode() == BacklightingMode_Heat) {
        uint8_t keyId = Utils_KeyStateToKeyId(keystate);

        Heat[keyId]++;
        HeatSum++;

        if (Heat[keyId] > HeatMax) {
            HeatMax = Heat[keyId];
        }

        if (Heat[keyId] >= RETAIN) {
            renormalize();
        }

        Heatmap_SetKeyColors();

        EventVector_Set(EventVector_LedMapUpdateNeeded);
        if (DEVICE_IS_UHK80_RIGHT) {
#ifdef __ZEPHYR__
            StateSync_UpdateLayer(LayerId_Base, true);
#endif
        }
    }
}

static uint8_t baseColor(uint8_t v, uint8_t base, int8_t scale) {
    int16_t center = 128;
    int16_t diff = (((int16_t)v) - center) / 2;
    int16_t scaled = diff*scale + base;
    scaled = scaled < 0 ? 0 : scaled;
    scaled = MIN(255, scaled);
    return (uint8_t)scaled;
}

static rgb_t redBlueHeat(uint8_t heatValue) {
    uint8_t base = 128;
    return (rgb_t) {
        .red = baseColor(heatValue, base, 3),
        .green = baseColor(heatValue, base/2, -1),
        .blue = baseColor(heatValue, base/4, -2)
    };
}

static rgb_t brightness(rgb_t color, uint8_t brightness) {
    uint16_t max1 = MAX(color.red, color.green);
    uint16_t max2 = MAX(max1, color.blue);
    uint16_t sum = color.red + color.green + color.blue + 1;
    uint16_t maxScaled = max2 * 3 * 255 / sum;

    if (maxScaled > 255) {
        sum = max2 * 3 + 1;
    }

    return (rgb_t) {
        .red = MIN(255, color.red * 3 * brightness / sum),
        .green = MIN(255, color.green * 3 * brightness / sum),
        .blue = MIN(255, color.blue * 3 * brightness / sum),
    };
}

static rgb_t saturate(rgb_t color, uint8_t saturation) {
    uint8_t saturationInv = 255 - saturation;
    uint8_t gray = (color.red + color.green + color.blue) / 3;
    return (rgb_t) {
        .red = (color.red * saturation + gray * saturationInv*3/2) / 255,
        .green = (color.green * saturation + gray * saturationInv) / 255,
        .blue = (color.blue * saturation + gray * saturationInv/2) / 255
    };
}

static rgb_t weightedAverage(rgb_t c1, uint8_t weightC2, rgb_t c2) {
    uint8_t weightC1 = 255 - weightC2;
    return (rgb_t) {
        .red = (c1.red * weightC1 + c2.red * weightC2) / 255,
        .green = (c1.green * weightC1 + c2.green * weightC2) / 255,
        .blue = (c1.blue * weightC1 + c2.blue * weightC2) / 255
    };
}

static rgb_t burn(rgb_t color, uint16_t heat) {
    if (heat >= 196) {
        color.red = MIN(255, color.red + (heat - 196));
        color.blue = MIN(255, color.blue + (heat - 196)/2);
        color.green = MIN(255, color.green + (heat - 196)/4);
    }
    return color;
}

void Heatmap_SetKeyColors(void) {
    uint16_t avgHeat = HeatSum / KEYCOUNT + 1;

    if (DEVICE_IS_UHK80_RIGHT || DEVICE_IS_UHK60) {
        for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
            for (uint8_t keyIndex=0; keyIndex<MAX_KEY_COUNT_PER_MODULE; keyIndex++) {
                uint8_t keyId = keyIndex + slotId*64;
                uint16_t heat = Heat[keyId] * 255 / (2*avgHeat);
                heat = MIN(255, heat);
                uint16_t heatSqr = (heat * heat) / 255;

                ATTR_UNUSED rgb_t redBlue = saturate(brightness(redBlueHeat(heat), heat), 200);
                ATTR_UNUSED rgb_t warmWhite = brightness((rgb_t){255, 192, 16}, heat);
                ATTR_UNUSED rgb_t flame = brightness((rgb_t){255, 128, 16}, heat);
                flame.red = MAX(64, flame.red);
                flame = brightness(flame, heat);
                flame = burn(flame, heat);

                ATTR_UNUSED rgb_t redBlue2 = brightness(redBlueHeat(heat), heatSqr);
                ATTR_UNUSED rgb_t warmWhite2 = brightness((rgb_t){255, 192, 16}, heatSqr);


                ATTR_UNUSED rgb_t c1 = warmWhite;
                ATTR_UNUSED rgb_t c2 = weightedAverage(warmWhite, 128, flame);
                ATTR_UNUSED rgb_t c3 = weightedAverage(warmWhite, 128, redBlue);
                ATTR_UNUSED rgb_t c4 = redBlue2;
                ATTR_UNUSED rgb_t c5 = weightedAverage(warmWhite2, 128, redBlue2);


                key_action_t* keyAction = &CurrentKeymap[LayerId_Base][slotId][keyIndex];

                if (!keyAction->colorOverridden) {
                    keyAction->color = c4;
                }
            }
        }
    }
}

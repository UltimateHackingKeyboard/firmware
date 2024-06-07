#include "led_manager.h"
#include "usb_composite_device.h"
#include "config_manager.h"
#include "stubs.h"

#ifdef __ZEPHYR__
#include <zephyr/sys/util.h>
#include "keyboard/state_sync.h"
#include "keyboard/power.h"
#include "keyboard/oled/oled.h"
#define SleepModeActive false
#else
#include "device/device.h"
#endif

bool KeyBacklightSleepModeActive = false;
bool DisplaySleepModeActive = false;

bool LedManager_FullUpdateNeeded = false;

uint8_t DisplayBrightness = 0xff;
uint8_t KeyBacklightBrightness = 0xff;

static void recalculateLedBrightness()
{
    bool globalSleepMode = !Cfg.LedsEnabled || SleepModeActive || Cfg.LedBrightnessMultiplier == 0.0f;

    if (globalSleepMode || KeyBacklightSleepModeActive) {
        KeyBacklightBrightness = 0;
    } else {
        uint8_t keyBacklightBrightnessBase = RunningOnBattery ? Cfg.KeyBacklightBrightnessBatteryDefault : Cfg.KeyBacklightBrightnessDefault;
        KeyBacklightBrightness = MIN(255, keyBacklightBrightnessBase * Cfg.LedBrightnessMultiplier);
    }

    if (globalSleepMode || DisplaySleepModeActive) {
        DisplayBrightness = 0;
    } else {
        uint8_t displayBrightnessBase = RunningOnBattery ? Cfg.DisplayBrightnessBatteryDefault : Cfg.DisplayBrightnessDefault;
        DisplayBrightness = MIN(255, displayBrightnessBase * Cfg.LedBrightnessMultiplier);
    }
}

void LedManager_FullUpdate()
{
    recalculateLedBrightness();
    Ledmap_UpdateBacklightLeds();

#ifdef __ZEPHYR__
    Oled_UpdateBrightness();
    StateSync_UpdateProperty(StateSyncPropertyId_Backlight, NULL);
#else
    LedDisplay_UpdateAll();
#endif

    LedManager_FullUpdateNeeded = false;
}

void LedManager_RecalculateLedBrightness()
{
    recalculateLedBrightness();
}


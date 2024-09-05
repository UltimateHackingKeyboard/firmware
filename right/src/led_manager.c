#include "led_manager.h"
#include "event_scheduler.h"
#include "usb_composite_device.h"
#include "config_manager.h"
#include "stubs.h"
#include "led_display.h"
#include "usb_report_updater.h"
#include "test_switches.h"

#ifdef __ZEPHYR__
#include <zephyr/sys/util.h>
#include "state_sync.h"
#include "keyboard/charger.h"
#include "keyboard/oled/oled.h"
#define SleepModeActive false
#else
#include "device.h"
#endif

bool KeyBacklightSleepModeActive = false;
bool DisplaySleepModeActive = false;

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
    EventVector_Unset(EventVector_LedManagerFullUpdateNeeded);
    EventVector_Unset(EventVector_LedMapUpdateNeeded);

    LedManager_UpdateSleepModes();
    recalculateLedBrightness();
    Ledmap_UpdateBacklightLeds();

#ifdef __ZEPHYR__
    Oled_UpdateBrightness();
    StateSync_UpdateProperty(StateSyncPropertyId_Backlight, NULL);
#else
    LedDisplay_UpdateAll();
#endif
}

void LedManager_RecalculateLedBrightness()
{
    recalculateLedBrightness();
}

void LedManager_UpdateAgentLed()
{
#ifndef __ZEPHYR
    const uint32_t updatePeriod = 1000;
    if (!TestSwitches) {
       LedDisplay_SetIcon(LedDisplayIcon_Agent, CurrentTime - LastUsbGetKeyboardStateRequestTimestamp < 1000);
    }
    EventScheduler_Schedule(CurrentTime + updatePeriod, EventSchedulerEvent_AgentLed, "Agent led");
#endif
}

void LedManager_UpdateSleepModes() {
    uint32_t elapsedTime = Timer_GetElapsedTime(&UsbReportUpdater_LastActivityTime);
    bool ledsNeedUpdate = false;

    uint32_t keyBacklightTimeout = RunningOnBattery ? Cfg.KeyBacklightFadeOutBatteryTimeout : Cfg.KeyBacklightFadeOutTimeout;
    if (elapsedTime >= keyBacklightTimeout && !KeyBacklightSleepModeActive && keyBacklightTimeout) {
        KeyBacklightSleepModeActive = true;
        ledsNeedUpdate = true;
    } else if ((elapsedTime < keyBacklightTimeout || !keyBacklightTimeout)  && KeyBacklightSleepModeActive) {
        KeyBacklightSleepModeActive = false;
        ledsNeedUpdate = true;
    }

    if (!KeyBacklightSleepModeActive && keyBacklightTimeout) {
        EventScheduler_Reschedule(UsbReportUpdater_LastActivityTime + keyBacklightTimeout, EventSchedulerEvent_UpdateLedSleepModes, "LedManager - update key backlight sleep mode");
    }

    uint32_t displayTimeout = RunningOnBattery ? Cfg.DisplayFadeOutBatteryTimeout : Cfg.DisplayFadeOutTimeout;
    if (elapsedTime >= displayTimeout && !DisplaySleepModeActive && displayTimeout) {
        DisplaySleepModeActive = true;
        ledsNeedUpdate = true;
    } else if ((elapsedTime < displayTimeout || !displayTimeout) && DisplaySleepModeActive) {
        DisplaySleepModeActive = false;
        ledsNeedUpdate = true;
    }

    if (!DisplaySleepModeActive && displayTimeout) {
        EventScheduler_Reschedule(UsbReportUpdater_LastActivityTime + displayTimeout, EventSchedulerEvent_UpdateLedSleepModes, "LedManager - update display sleep mode");
    }

    if (ledsNeedUpdate) {
        EventVector_Set(EventVector_LedManagerFullUpdateNeeded);
    }
}


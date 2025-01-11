#include "led_manager.h"
#include "event_scheduler.h"
#include "ledmap.h"
#include "usb_composite_device.h"
#include "config_manager.h"
#include "stubs.h"
#include "led_display.h"
#include "usb_report_updater.h"
#include "test_switches.h"
#include "power_mode.h"

#ifdef __ZEPHYR__
#include <zephyr/sys/util.h>
#include "state_sync.h"
#include "keyboard/charger.h"
#include "keyboard/oled/oled.h"
#else
#include "device.h"
#endif

#if DEVICE_IS_UHK80_RIGHT
#include "keyboard/oled/screens/screen_manager.h"
#else
#define InteractivePairingInProgress false;
#endif

bool KeyBacklightSleepModeActive = false;
bool DisplaySleepModeActive = false;

uint8_t DisplayBrightness = 0xff;
uint8_t KeyBacklightBrightness = 0xff;

static void recalculateLedBrightness()
{
    bool globalSleepMode = !Cfg.LedsEnabled || CurrentPowerMode > PowerMode_Awake || Cfg.LedBrightnessMultiplier == 0.0f;
    bool globalAlwaysOn = Cfg.LedsAlwaysOn || Ledmap_AlwaysOn || InteractivePairingInProgress;

    if (!globalAlwaysOn && (globalSleepMode || KeyBacklightSleepModeActive)) {
        KeyBacklightBrightness = 0;
    } else {
        uint8_t keyBacklightBrightnessBase = RunningOnBattery ? Cfg.KeyBacklightBrightnessBatteryDefault : Cfg.KeyBacklightBrightnessDefault;
        KeyBacklightBrightness = MIN(255, keyBacklightBrightnessBase * Cfg.LedBrightnessMultiplier);
    }

    if (!globalAlwaysOn && (globalSleepMode || DisplaySleepModeActive)) {
        DisplayBrightness = 0;
    } else {
        uint8_t displayBrightnessBase = RunningOnBattery ? Cfg.DisplayBrightnessBatteryDefault : Cfg.DisplayBrightnessDefault;
        DisplayBrightness = MIN(255, displayBrightnessBase * Cfg.LedBrightnessMultiplier);
    }

    if (Ledmap_AlwaysOn) {
        KeyBacklightBrightness = 255;
    }
}

void LedManager_FullUpdate()
{
    EventVector_Unset(EventVector_LedManagerFullUpdateNeeded);
    EventVector_Unset(EventVector_LedMapUpdateNeeded);

    if (DEVICE_IS_UHK80_LEFT) {
        Ledmap_UpdateBacklightLeds();
        return;
    }

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

    EventScheduler_Unschedule(EventSchedulerEvent_UpdateLedSleepModes);

    uint32_t keyBacklightTimeout = RunningOnBattery ? Cfg.KeyBacklightFadeOutBatteryTimeout : Cfg.KeyBacklightFadeOutTimeout;
    if (elapsedTime >= keyBacklightTimeout && !KeyBacklightSleepModeActive && keyBacklightTimeout) {
        KeyBacklightSleepModeActive = true;
        ledsNeedUpdate = true;
    } else if ((elapsedTime < keyBacklightTimeout || !keyBacklightTimeout)  && KeyBacklightSleepModeActive) {
        KeyBacklightSleepModeActive = false;
        ledsNeedUpdate = true;
    }

    if (!KeyBacklightSleepModeActive && keyBacklightTimeout) {
        EventScheduler_Schedule(UsbReportUpdater_LastActivityTime + keyBacklightTimeout, EventSchedulerEvent_UpdateLedSleepModes, "LedManager - update key backlight sleep mode");
    }

    uint32_t displayTimeout = RightRunningOnBattery ? Cfg.DisplayFadeOutBatteryTimeout : Cfg.DisplayFadeOutTimeout;
    if (elapsedTime >= displayTimeout && !DisplaySleepModeActive && displayTimeout) {
        DisplaySleepModeActive = true;
        ledsNeedUpdate = true;
    } else if ((elapsedTime < displayTimeout || !displayTimeout) && DisplaySleepModeActive) {
        DisplaySleepModeActive = false;
        ledsNeedUpdate = true;
    }

    if (!DisplaySleepModeActive && displayTimeout) {
        EventScheduler_Schedule(UsbReportUpdater_LastActivityTime + displayTimeout, EventSchedulerEvent_UpdateLedSleepModes, "LedManager - update display sleep mode");
    }

    if (ledsNeedUpdate) {
        EventVector_Set(EventVector_LedManagerFullUpdateNeeded);
    }
}


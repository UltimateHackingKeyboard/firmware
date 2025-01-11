#include "screen_manager.h"
#include "canvas_screen.h"
#include "keyboard/oled/widgets/widgets.h"
#include "keyboard/oled/screens/screens.h"
#include "keyboard/oled/oled.h"
#include "timer.h"
#include "event_scheduler.h"
#include "timer.h"
#include "event_scheduler.h"
#include "ledmap.h"

screen_id_t ActiveScreen = ScreenId_Main;

bool InteractivePairingInProgress = false;

static void onExit(screen_id_t screen) {
    switch(screen) {
        case ScreenId_Pairing:
            InteractivePairingInProgress = false;
            Ledmap_ResetTemporaryLedBacklightingMode();
            EventVector_Set(EventVector_LedManagerFullUpdateNeeded);
            break;
        default:
            break;
    }
}

void ScreenManager_ActivateScreen(screen_id_t screen)
{
    widget_t* screenPtr = NULL;

    onExit(ActiveScreen);

    switch(screen) {
        case ScreenId_Pairing:
            InteractivePairingInProgress = true;
            screenPtr = PairingScreen;
            Ledmap_SetTemporaryLedBacklightingMode(BacklightingMode_Numpad);
            EventVector_Set(EventVector_LedManagerFullUpdateNeeded);
            Ledmap_UpdateBacklightLeds();
            break;
        case ScreenId_Debug:
            screenPtr = DebugScreen;
            EventScheduler_Reschedule(CurrentTime + DEBUG_SCREEN_NOTIFICATION_TIMEOUT, EventSchedulerEvent_SwitchScreen, "ScreenManager - switch to main screen");
            break;
        case ScreenId_Main:
            screenPtr = MainScreen;
            break;
        case ScreenId_PairingSucceeded:
            screenPtr = PairingSucceededScreen;
            EventScheduler_Schedule(CurrentTime + SCREEN_NOTIFICATION_TIMEOUT, EventSchedulerEvent_SwitchScreen, "ScreenManager - switch to main screen");
            break;
        case ScreenId_PairingFailed:
            screenPtr = PairingFailedScreen;
            EventScheduler_Schedule(CurrentTime + SCREEN_NOTIFICATION_TIMEOUT, EventSchedulerEvent_SwitchScreen, "ScreenManager - switch to main screen");
            break;
        case ScreenId_Canvas:
            EventScheduler_Reschedule(CurrentTime + CANVAS_TIMEOUT, EventSchedulerEvent_SwitchScreen, "ScreenManager - switch to main screen");
            screenPtr = CanvasScreen;
            break;
        default:
            break;
    }

    ActiveScreen = screen;
    Oled_ActivateScreen(screenPtr, false);
    Oled_RequestRedraw();
}

void ScreenManager_SwitchScreenEvent()
{
    ScreenManager_ActivateScreen(ScreenId_Main);
}

void ScreenManager_Init()
{
    WidgetStore_Init();
    PairingScreen_Init();
    CanvasScreen_Init();
    MainScreen_Init();
    DebugScreen_Init();
}

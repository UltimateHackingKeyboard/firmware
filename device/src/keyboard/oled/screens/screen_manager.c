#include "screen_manager.h"
#include "canvas_screen.h"
#include "keyboard/oled/widgets/widgets.h"
#include "keyboard/oled/screens/screens.h"
#include "keyboard/oled/oled.h"
#include "notification_screen.h"
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
            EventScheduler_Reschedule(Timer_GetCurrentTime() + DEBUG_SCREEN_NOTIFICATION_TIMEOUT, EventSchedulerEvent_SwitchScreen, "ScreenManager - switch to main screen");
            break;
        case ScreenId_Main:
            screenPtr = MainScreen;
            break;
        case ScreenId_Canvas:
            if (!OledOverrideMode) {
                EventScheduler_Reschedule(Timer_GetCurrentTime() + CANVAS_TIMEOUT, EventSchedulerEvent_SwitchScreen, "ScreenManager - switch to main screen");
            }
            screenPtr = CanvasScreen;
            break;
        case ScreenId_Notification:
            screenPtr = NotificationScreen;
            break;
        default:
            break;
    }

    ActiveScreen = screen;
    Oled_ActivateScreen(screenPtr, false);
    // synchronous version leads to a deadlock from non-main threads
    if (k_current_get() == Main_ThreadId) {
        Oled_RequestRedraw();
    } else {
        EventScheduler_Schedule(Timer_GetCurrentTime() + 10, EventSchedulerEvent_RedrawOled, "Activate screen");
    }
}

void ScreenManager_SwitchScreenEvent()
{
    if (OledOverrideMode) {
        ScreenManager_ActivateScreen(ScreenId_Canvas);
    } else {
        ScreenManager_ActivateScreen(ScreenId_Main);
    }
}

void ScreenManager_Init()
{
    WidgetStore_Init();
    PairingScreen_Init();
    CanvasScreen_Init();
    MainScreen_Init();
    DebugScreen_Init();
    NotificationScreen_Init();
}

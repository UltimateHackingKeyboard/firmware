#include "screen_manager.h"
#include "canvas_screen.h"
#include "keyboard/oled/widgets/widgets.h"
#include "keyboard/oled/screens/screens.h"
#include "keyboard/oled/oled.h"
#include "legacy/timer.h"
#include "legacy/event_scheduler.h"
#include "legacy/timer.h"
#include "legacy/event_scheduler.h"

screen_id_t ActiveScreen = ScreenId_Main;

void ScreenManager_ActivateScreen(screen_id_t screen)
{
    widget_t* screenPtr = NULL;

    switch(screen) {
        case ScreenId_Pairing:
            screenPtr = PairingScreen;
            break;
        case ScreenId_Debug:
            screenPtr = DebugScreen;
            EventScheduler_Reschedule(CurrentTime + DEBUG_SCREEN_NOTIFICATION_TIMEOUT, EventSchedulerEvent_SwitchScreen);
            break;
        case ScreenId_Main:
            screenPtr = MainScreen;
            break;
        case ScreenId_PairingSucceeded:
            screenPtr = PairingSucceededScreen;
            EventScheduler_Schedule(CurrentTime + SCREEN_NOTIFICATION_TIMEOUT, EventSchedulerEvent_SwitchScreen);
            break;
        case ScreenId_PairingFailed:
            screenPtr = PairingFailedScreen;
            EventScheduler_Schedule(CurrentTime + SCREEN_NOTIFICATION_TIMEOUT, EventSchedulerEvent_SwitchScreen);
            break;
        case ScreenId_Canvas:
            EventScheduler_Reschedule(CurrentTime + CANVAS_TIMEOUT, EventSchedulerEvent_SwitchScreen);
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

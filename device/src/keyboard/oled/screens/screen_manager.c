#include "screen_manager.h"
#include "canvas_screen.h"
#include "keyboard/oled/widgets/widgets.h"
#include "keyboard/oled/oled.h"
#include "pairing_screen.h"
#include "test_screen.h"
#include "canvas_screen.h"
#include "legacy/timer.h"
#include "legacy/event_scheduler.h"

screen_id_t ActiveScreen = ScreenId_Test;

void ScreenManager_ActivateScreen(screen_id_t screen)
{
    widget_t* screenPtr = NULL;

    switch(screen) {
        case ScreenId_Pairing:
            screenPtr = PairingScreen;
            break;
        case ScreenId_Test:
            screenPtr = TestScreen;
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
            EventScheduler_Schedule(CurrentTime + CANVAS_TIMEOUT, EventSchedulerEvent_SwitchScreen);
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
    ScreenManager_ActivateScreen(ScreenId_Test);
}

void ScreenManager_Init()
{
    WidgetStore_Init();
    PairingScreen_Init();
    TestScreen_Init();
    CanvasScreen_Init();
}

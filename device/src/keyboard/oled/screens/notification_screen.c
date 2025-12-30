#include "notification_screen.h"
#include "keyboard/oled/widgets/widgets.h"
#include "keyboard/oled/screens/screen_manager.h"
#include "screen_manager.h"
#include <zephyr/kernel.h>
#include "event_scheduler.h"

static widget_t notificationWidget;

widget_t* NotificationScreen;

uint32_t NotificationDuration = 0;

void NotificationScreen_Init(void) {
    notificationWidget = TextWidget_Build(&JetBrainsMono16, "Hello!");
    NotificationScreen = &notificationWidget;
}

void NotificationScreen_NotifyFor(const char* message, uint16_t duration) {
    printk("Notification: %s\n", message);
    TextWidget_SetText(&notificationWidget, message);
    ScreenManager_ActivateScreen(ScreenId_Notification);

    if (duration != 0) {
        EventScheduler_Reschedule(Timer_GetCurrentTime() + duration, EventSchedulerEvent_SwitchScreen, "notification screen - switch to main screen");
    }
}

void NotificationScreen_Notify(const char* message) {
    NotificationScreen_NotifyFor(message, SCREEN_NOTIFICATION_TIMEOUT);
}

void NotificationScreen_Hide() {
    EventScheduler_Reschedule(Timer_GetCurrentTime() + 10, EventSchedulerEvent_SwitchScreen, "notification screen - hide");
}


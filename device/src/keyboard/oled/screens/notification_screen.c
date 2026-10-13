#include "notification_screen.h"
#include "keyboard/oled/widgets/widgets.h"
#include "keyboard/oled/screens/screen_manager.h"
#include "keyboard/oled/oled.h"
#include "screen_manager.h"
#include <zephyr/kernel.h>
#include "event_scheduler.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Notify, LOG_LEVEL_INF);

// Let the display fade out while the notification is still shown.
#define FORCE_AWAKE_FADE_OUT_TIME (OLED_FADE_TIME_WITH_MARGIN)

static widget_t notificationWidget;
static uint8_t currentSeqId = NotificationId_Invalid;

widget_t* NotificationScreen;

uint32_t NotificationDuration = 0;

bool NotificationScreen_ForceAwake = false;

void NotificationScreen_Init(void) {
    notificationWidget = TextWidget_Build(&JetBrainsMono16, "Hello!");
    NotificationScreen = &notificationWidget;
}

static uint8_t nextSeqId(void) {
    currentSeqId = currentSeqId >= NotificationId_LastValid ? NotificationId_FirstValid : currentSeqId + 1;
    return currentSeqId;
}

static void setForceAwake(bool forceAwake, uint16_t duration) {
    NotificationScreen_ForceAwake = forceAwake;

    if (forceAwake && duration != 0) {
        uint16_t awakeTime = duration > FORCE_AWAKE_FADE_OUT_TIME ? duration - FORCE_AWAKE_FADE_OUT_TIME : 0;
        EventScheduler_Reschedule(Timer_GetCurrentTime() + awakeTime, EventSchedulerEvent_NotificationForceAwakeEnd, "notification screen - end force awake");
    } else {
        EventScheduler_Unschedule(EventSchedulerEvent_NotificationForceAwakeEnd);
    }
}

void NotificationScreen_EndForceAwake(void) {
    NotificationScreen_ForceAwake = false;
    Oled_RequestRedraw();
}

uint8_t NotificationScreen_NotifyFor(const char* message, uint16_t duration, bool forceAwake) {
    LOG_INF("Notification: %s", message);
    uint8_t seqId = nextSeqId();
    setForceAwake(forceAwake, duration);
    TextWidget_SetText(&notificationWidget, message);
    ScreenManager_ActivateScreen(ScreenId_Notification);

    if (duration != 0) {
        EventScheduler_Reschedule(Timer_GetCurrentTime() + duration, EventSchedulerEvent_SwitchScreen, "notification screen - switch to main screen");
    }

    return seqId;
}

uint8_t NotificationScreen_Notify(const char* message) {
    return NotificationScreen_NotifyFor(message, SCREEN_NOTIFICATION_TIMEOUT, false);
}

void NotificationScreen_Hide(uint8_t seqId) {
    bool matches = seqId == NotificationId_Any || (seqId != NotificationId_Invalid && seqId == currentSeqId);
    bool isShown = matches && ActiveScreen == ScreenId_Notification;

    if (isShown) {
        setForceAwake(false, 0);
        EventScheduler_Reschedule(Timer_GetCurrentTime() + 10, EventSchedulerEvent_SwitchScreen, "notification screen - hide");
    }
}


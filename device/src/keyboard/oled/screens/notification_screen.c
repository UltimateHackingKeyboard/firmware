#include "notification_screen.h"
#include "keyboard/oled/widgets/widgets.h"
#include "keyboard/oled/screens/screen_manager.h"
#include "screen_manager.h"
#include <zephyr/kernel.h>

static widget_t notificationWidget;

widget_t* NotificationScreen;

void NotificationScreen_Init(void) {
    notificationWidget = TextWidget_Build(&JetBrainsMono16, "Hello!");
    NotificationScreen = &notificationWidget;
}

void NotificationScreen_Notify(const char* message) {
    printk("Notification: %s\n", message);
    TextWidget_SetText(&notificationWidget, message);
    ScreenManager_ActivateScreen(ScreenId_Notification);
}

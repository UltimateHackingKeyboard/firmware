#include "console_widget.h"
#include "custom_widget.h"
#include "keyboard/oled/widgets/custom_widget.h"
#include "keyboard/oled/oled.h"
#include "keyboard/oled/widgets/widgets.h"
#include "keyboard/oled/fonts/fonts.h"
#include "keyboard/oled/framebuffer.h"
#include "keyboard/oled/oled_text_renderer.h"
#include "text_widget.h"
#include "widget.h"
#include "legacy/keymap.h"
#include "legacy/layer.h"
#include "legacy/layer_switcher.h"
#include "legacy/str_utils.h"
#include "keyboard/uart.h"
#include "bt_conn.h"
#include "keyboard/state_sync.h"
#include <string.h>
#include <stdio.h>

widget_t KeymapWidget;
widget_t LayerWidget;
widget_t KeymapLayerWidget;
widget_t StatusWidget;
widget_t CanvasWidget;
widget_t ConsoleWidget;

static string_segment_t getLayerText() {
    return (string_segment_t){ .start = LayerNames[ActiveLayer], .end = NULL };
}

static string_segment_t getKeymapText() {
    if (strcmp(AllKeymaps[CurrentKeymapIndex].abbreviation, "FTY") == 0) {
        return (string_segment_t){ .start = "Factory default", .end = NULL };
    } else {
        return GetKeymapName(CurrentKeymapIndex);
    }
}

static string_segment_t getKeymapLayerText() {
#define BUFFER_LENGTH 32
    static char buffer[BUFFER_LENGTH] = { [BUFFER_LENGTH-1] = 0 };
    string_segment_t layerText = getLayerText();
    string_segment_t keymapText = getKeymapText();
    snprintf(buffer, BUFFER_LENGTH-1, "%.*s   %.*s", SegmentLen(keymapText), keymapText.start, SegmentLen(layerText), layerText.start);
    return (string_segment_t){ .start = buffer, .end = NULL };
#undef BUFFER_LENGTH
}

static string_segment_t getLeftStatusText() {
#define BUFFER_LENGTH 20
    static char buffer [BUFFER_LENGTH] = { [BUFFER_LENGTH-1] = 0 };
    const char* connState = "---";
    if (DEVICE_ID == DeviceId_Uhk80_Right) {
        if (Uart_IsConnected()) {
            connState = "uart";
        } else if (Bt_DeviceIsConnected(DeviceId_Uhk80_Left)) {
            connState = "bt";
        }
    }
    snprintf(buffer, BUFFER_LENGTH-1, "left: %s", connState);
    return (string_segment_t){ .start = buffer, .end = NULL };
#undef BUFFER_LENGTH
}

static void getBatteryStatusText(battery_state_t* battery, char* buffer) {
    if (!battery->batteryPresent) {
        sprintf(buffer, "n/a");
    } else if (battery->batteryCharging) {
        sprintf(buffer, "%i+", battery->batteryPercentage);
    } else {
        sprintf(buffer, "%i%%", battery->batteryPercentage);
    }
}

static string_segment_t getRightStatusText() {
#define BUFFER_LENGTH 20
    static char buffer [BUFFER_LENGTH] = { [BUFFER_LENGTH-1] = 0 };
    char leftBattery[5];
    char rightBattery[5];
    getBatteryStatusText(&SyncLeftHalfState.battery, leftBattery);
    getBatteryStatusText(&SyncRightHalfState.battery, rightBattery);
    snprintf(buffer, BUFFER_LENGTH-1, "%s %s", leftBattery, rightBattery);
    return (string_segment_t){ .start = buffer, .end = NULL };
#undef BUFFER_LENGTH
}

static void drawStatus(widget_t* self, framebuffer_t* buffer)
{
    if (self->dirty) {
        self->dirty = false;
        Framebuffer_Clear(self, buffer);
        Framebuffer_DrawTextAnchored(self, buffer, AnchorType_Begin, AnchorType_Center, &JetBrainsMono8, getLeftStatusText().start, NULL);
        Framebuffer_DrawTextAnchored(self, buffer, AnchorType_End, AnchorType_Center, &JetBrainsMono8, getRightStatusText().start, NULL);
    }
}

void WidgetStore_Init()
{
    LayerWidget = TextWidget_BuildRefreshable(&JetBrainsMono16, &getLayerText);
    KeymapWidget = TextWidget_BuildRefreshable(&JetBrainsMono16, &getKeymapText);
    KeymapLayerWidget = TextWidget_BuildRefreshable(&JetBrainsMono16, &getKeymapLayerText);
    StatusWidget = CustomWidget_Build(&drawStatus);
    CanvasWidget = CustomWidget_Build(NULL);
    ConsoleWidget = ConsoleWidget_Build();
}

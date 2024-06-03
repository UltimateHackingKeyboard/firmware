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
#include <string.h>
#include <stdio.h>

widget_t KeymapWidget;
widget_t LayerWidget;
widget_t StatusWidget;
widget_t CanvasWidget;

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

static string_segment_t getStatusText() {
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
}

void WidgetStore_Init()
{
    LayerWidget = TextWidget_BuildRefreshable(&JetBrainsMono16, &getLayerText);
    KeymapWidget = TextWidget_BuildRefreshable(&JetBrainsMono16, &getKeymapText);
    StatusWidget = TextWidget_BuildRefreshable(&JetBrainsMono8, &getStatusText);
    CanvasWidget = CustomWidget_Build(NULL);
}

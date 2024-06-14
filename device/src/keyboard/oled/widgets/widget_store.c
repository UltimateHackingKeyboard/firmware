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
#include "state_sync.h"
#include <string.h>
#include <stdio.h>
#include "device_state.h"
#include "usb/usb_compatibility.h"

widget_t KeymapWidget;
widget_t LayerWidget;
widget_t KeymapLayerWidget;
widget_t StatusWidget;
widget_t CanvasWidget;
widget_t ConsoleWidget;
widget_t TargetWidget;

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


static string_segment_t getTargetText() {
    if (DeviceState_IsConnected(ConnectionId_UsbHid)) {
        return (string_segment_t){ .start = "USB Cable", .end = NULL };
    } else if (DeviceState_IsConnected(ConnectionId_Dongle)) {
        return (string_segment_t){ .start = "UHK Dongle", .end = NULL };
    } else if (DeviceState_IsConnected(ConnectionId_BluetoothHid)) {
        return (string_segment_t){ .start = "Bluetooth", .end = NULL };
    } else {
        return (string_segment_t){ .start = "Disconnected", .end = NULL };
    }
}


ATTR_UNUSED static string_segment_t getKeymapLayerText() {
#define BUFFER_LENGTH 32
    static char buffer[BUFFER_LENGTH] = { [BUFFER_LENGTH-1] = 0 };
    string_segment_t layerText = getLayerText();
    string_segment_t keymapText = getKeymapText();
    if (ActiveLayer == LayerId_Base) {
        snprintf(buffer, BUFFER_LENGTH-1, "%.*s", SegmentLen(keymapText), keymapText.start);
    } else {
        snprintf(buffer, BUFFER_LENGTH-1, "%.*s: %.*s", SegmentLen(keymapText), keymapText.start, SegmentLen(layerText), layerText.start);
    }
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

static void getBatteryStatusText(device_id_t deviceId, battery_state_t* battery, char* buffer, bool fixed) {
    char percSign = !battery->powered ? '-' : battery->batteryCharging ? '+' : '%';
    if (!DeviceState_IsDeviceConnected(deviceId)) {
        sprintf(buffer, "    ");
    } else if (!battery->batteryPresent) {
        sprintf(buffer, "    ");
    } else {
        sprintf(buffer, fixed ? "%3i%c" : "%i%c", battery->batteryPercentage, percSign);
    }
}

static string_segment_t getRightStatusText() {
#define BUFFER_LENGTH 22
#define BAT_BUFFER_LENGTH 10
    static char buffer [BUFFER_LENGTH] = { [BUFFER_LENGTH-1] = 0 };
    char leftBattery[BAT_BUFFER_LENGTH];
    char rightBattery[BAT_BUFFER_LENGTH];
    if (SyncLeftHalfState.battery.batteryPresent && SyncRightHalfState.battery.batteryPresent) {
        getBatteryStatusText(DeviceId_Uhk80_Left, &SyncLeftHalfState.battery, leftBattery, true);
        getBatteryStatusText(DeviceId_Uhk80_Right, &SyncRightHalfState.battery, rightBattery, true);
        snprintf(buffer, BUFFER_LENGTH-1, "%s %s", leftBattery, rightBattery);
    } else if (SyncLeftHalfState.battery.batteryPresent) {
        getBatteryStatusText(DeviceId_Uhk80_Left, &SyncLeftHalfState.battery, leftBattery, false);
        snprintf(buffer, BUFFER_LENGTH-1, "L%s", leftBattery);
    } else if (SyncRightHalfState.battery.batteryPresent) {
        getBatteryStatusText(DeviceId_Uhk80_Right, &SyncRightHalfState.battery, rightBattery, false);
        snprintf(buffer, BUFFER_LENGTH-1, "R%s", rightBattery);
    } else {
        snprintf(buffer, BUFFER_LENGTH-1, "");
    }
    return (string_segment_t){ .start = buffer, .end = NULL };
#undef BAT_BUFFER_LENGTH
#undef BUFFER_LENGTH
}

static string_segment_t getKeyboardLedsStateText() {
    static char buffer [6] = {};
    sprintf(buffer, "%cC %cN", KeyboardLedsState.capsLock ? FontControl_WhiteText : FontControl_GrayText, KeyboardLedsState.numLock ? FontControl_WhiteText : FontControl_GrayText);
    return (string_segment_t){ .start = buffer, .end = NULL };
}


static void drawStatus(widget_t* self, framebuffer_t* buffer)
{
    if (self->dirty) {
        self->dirty = false;
        Framebuffer_Clear(self, buffer);
        Framebuffer_DrawText(self, buffer, AlignmentType_Begin, AlignmentType_Center, &JetBrainsMono12, getLeftStatusText().start, NULL);
        Framebuffer_DrawText(self, buffer, AlignmentType_Center, AlignmentType_Center, &JetBrainsMono12, getKeyboardLedsStateText().start, NULL);
        Framebuffer_DrawText(self, buffer, AlignmentType_End, AlignmentType_Center, &JetBrainsMono12, getRightStatusText().start, NULL);
    }
}

static void drawKeymapLayer(widget_t* self, framebuffer_t* buffer)
{
    if (self->dirty) {
        self->dirty = false;
        Framebuffer_Clear(self, buffer);
        const lv_font_t* keymapFont = &JetBrainsMono16;
        const lv_font_t* layerFont = &JetBrainsMono12;
        string_segment_t keymapText = getKeymapText();
        string_segment_t layerText = getLayerText();
        uint16_t keymapWidth = Framebuffer_TextWidth(keymapFont, keymapText.start, keymapText.end);
        Framebuffer_DrawText(self, buffer, self->w/2 - keymapWidth/2, AlignmentType_Center, keymapFont, keymapText.start, keymapText.end);
        if (ActiveLayer != LayerId_Base) {
            Framebuffer_DrawText(self, buffer, self->w/2 + keymapWidth/2 + 10, AlignmentType_Center+(keymapFont->line_height - layerFont->line_height)/2, layerFont, layerText.start, layerText.end);
        }
    }
}

void WidgetStore_Init()
{
    LayerWidget = TextWidget_BuildRefreshable(&JetBrainsMono16, &getLayerText);
    KeymapWidget = TextWidget_BuildRefreshable(&JetBrainsMono24, &getKeymapText);
    TargetWidget = TextWidget_BuildRefreshable(&JetBrainsMono12, &getTargetText);
    KeymapLayerWidget = CustomWidget_Build(&drawKeymapLayer);
    StatusWidget = CustomWidget_Build(&drawStatus);
    CanvasWidget = CustomWidget_Build(NULL);
    ConsoleWidget = ConsoleWidget_Build();
}

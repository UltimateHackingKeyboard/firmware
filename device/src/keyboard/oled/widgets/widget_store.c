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
        return (string_segment_t){ .start = "Usb Cable", .end = NULL };
    } else if (DeviceState_IsConnected(ConnectionId_Dongle)) {
        return (string_segment_t){ .start = "Uhk Dongle", .end = NULL };
    } else if (DeviceState_IsConnected(ConnectionId_BluetoothHid)) {
        return (string_segment_t){ .start = "Bluetooth", .end = NULL };
    } else {
        return (string_segment_t){ .start = "Disconnected", .end = NULL };
    }
}


static string_segment_t getKeymapLayerText() {
#define BUFFER_LENGTH 32
    if (false) {
        static char buffer[BUFFER_LENGTH] = { [BUFFER_LENGTH-1] = 0 };
        string_segment_t layerText = getLayerText();
        string_segment_t keymapText = getKeymapText();
        snprintf(buffer, BUFFER_LENGTH-1, "%.*s   %.*s", SegmentLen(keymapText), keymapText.start, SegmentLen(layerText), layerText.start);
        return (string_segment_t){ .start = buffer, .end = NULL };
    } else {
        static char buffer[BUFFER_LENGTH] = { [BUFFER_LENGTH-1] = 0 };
        string_segment_t layerText = getLayerText();
        string_segment_t keymapText = getKeymapText();
        if (ActiveLayer == LayerId_Base) {
            snprintf(buffer, BUFFER_LENGTH-1, "%.*s", SegmentLen(keymapText), keymapText.start);
        } else {
            snprintf(buffer, BUFFER_LENGTH-1, "%.*s", SegmentLen(layerText), layerText.start);
        }
        return (string_segment_t){ .start = buffer, .end = NULL };
    }
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

static void getBatteryStatusText(device_id_t deviceId, battery_state_t* battery, char* buffer) {
    const char* powered = battery->powered ? "P" : "";
    if (!DeviceState_IsDeviceConnected(deviceId)) {
        sprintf(buffer, "---");
    } else if (!battery->batteryPresent) {
        sprintf(buffer, "n/a %s", powered);
    } else if (battery->batteryCharging) {
        sprintf(buffer, "%i+%s", battery->batteryPercentage, powered);
    } else {
        sprintf(buffer, "%i%%%s", battery->batteryPercentage, powered);
    }
}

static string_segment_t getRightStatusText() {
#define BUFFER_LENGTH 22
#define BAT_BUFFER_LENGTH 10
    static char buffer [BUFFER_LENGTH] = { [BUFFER_LENGTH-1] = 0 };
    char leftBattery[BAT_BUFFER_LENGTH];
    char rightBattery[BAT_BUFFER_LENGTH];
    getBatteryStatusText(DeviceId_Uhk80_Left, &SyncLeftHalfState.battery, leftBattery);
    getBatteryStatusText(DeviceId_Uhk80_Right, &SyncRightHalfState.battery, rightBattery);
    snprintf(buffer, BUFFER_LENGTH-1, "%s %s", leftBattery, rightBattery);
    return (string_segment_t){ .start = buffer, .end = NULL };
#undef BAT_BUFFER_LENGTH
#undef BUFFER_LENGTH
}

static string_segment_t getKeyboardLedsStateText() {
    static char buffer [6] = {};
    sprintf(buffer, "%cN %cC", KeyboardLedsState.numLock ? FontControl_WhiteText : FontControl_GrayText, KeyboardLedsState.capsLock ? FontControl_WhiteText : FontControl_GrayText);
    return (string_segment_t){ .start = buffer, .end = NULL };
}


static void drawStatus(widget_t* self, framebuffer_t* buffer)
{
    if (self->dirty) {
        self->dirty = false;
        Framebuffer_Clear(self, buffer);
        Framebuffer_DrawTextAnchored(self, buffer, AnchorType_Begin, AnchorType_Center, &JetBrainsMono12, getLeftStatusText().start, NULL);
        Framebuffer_DrawTextAnchored(self, buffer, AnchorType_Center, AnchorType_Center, &JetBrainsMono12, getKeyboardLedsStateText().start, NULL);
        Framebuffer_DrawTextAnchored(self, buffer, AnchorType_End, AnchorType_Center, &JetBrainsMono12, getRightStatusText().start, NULL);
    }
}

void WidgetStore_Init()
{
    LayerWidget = TextWidget_BuildRefreshable(&JetBrainsMono16, &getLayerText);
    KeymapWidget = TextWidget_BuildRefreshable(&JetBrainsMono24, &getKeymapText);
    KeymapLayerWidget = TextWidget_BuildRefreshable(&JetBrainsMono16, &getKeymapLayerText);
    TargetWidget = TextWidget_BuildRefreshable(&JetBrainsMono12, &getTargetText);
    StatusWidget = CustomWidget_Build(&drawStatus);
    CanvasWidget = CustomWidget_Build(NULL);
    ConsoleWidget = ConsoleWidget_Build();
}

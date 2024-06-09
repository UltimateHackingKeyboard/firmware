#include "main_screen.h"
#include "keyboard/oled/widgets/widgets.h"


static widget_t targetWidget;
static widget_t statusSplitter;
static widget_t keymapSplitter;
static widget_t keymapLayerSplitter;

widget_t* MainScreen;

static void drawTarget(widget_t* self, framebuffer_t* buffer)
{
    if (self->dirty) {
        self->dirty = false;
        Framebuffer_Clear(self, buffer);
        Framebuffer_DrawTextAnchored(self, buffer, AnchorType_Center, AnchorType_Center, &JetBrainsMono12, "My phone", NULL);
    }
}

void MainScreen_Init()
{
    const uint8_t statusHeight = 18;
    const uint8_t keymapHeight = 28;

    targetWidget = CustomWidget_Build(&drawTarget);
    keymapLayerSplitter = SplitterWidget_BuildHorizontal(&KeymapWidget, &LayerWidget, 128, false);
    keymapSplitter = SplitterWidget_BuildVertical(&KeymapLayerWidget, &targetWidget, keymapHeight, false);
    statusSplitter = SplitterWidget_BuildVertical(&StatusWidget, &keymapSplitter, statusHeight, false);

    MainScreen = &statusSplitter;
}

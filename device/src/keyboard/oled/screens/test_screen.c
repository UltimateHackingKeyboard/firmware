#include "test_screen.h"
#include "keyboard/oled/widgets/widgets.h"

static widget_t consoleWidget;
static widget_t helloWidget;
static widget_t frameWidget;
static widget_t splitterWidget;
static widget_t layerKeymapSplitter;
static widget_t layerWidget;
static widget_t keymapWidget;

widget_t* TestScreen;

static void drawHello(widget_t* self, framebuffer_t* buffer)
{
    if (self->dirty) {
        self->dirty = false;
        Framebuffer_Clear(self, buffer);
        Framebuffer_DrawTextAnchored(self, buffer, AnchorType_Center, AnchorType_Center, &JetBrainsMono32, "Hello!", NULL);
    }
}

void TestScreen_Init()
{
    helloWidget = CustomWidget_Build(&drawHello);
    layerWidget = LayerWidget_Build();
    keymapWidget = KeymapWidget_Build();
    consoleWidget = ConsoleWidget_Build();
    layerKeymapSplitter = SplitterWidget_BuildHorizontal(&keymapWidget, &layerWidget, 30, false);
    splitterWidget = SplitterWidget_BuildVertical(&consoleWidget, &layerKeymapSplitter, 120, true);
    frameWidget = FrameWidget_Build(&splitterWidget);
    TestScreen = &frameWidget;
}

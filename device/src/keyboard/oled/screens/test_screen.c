#include "test_screen.h"
#include "../framebuffer.h"
#include "../oled_text_renderer.h"
#include "../fonts/fonts.h"
#include "../widgets/custom_widget.h"
#include "../widgets/frame_widget.h"
#include "../widgets/console_widget.h"
#include "../widgets/splitter_widget.h"
#include "../widgets/widget.h"
#include "keyboard/logger.h"

static widget_t consoleWidget;
static widget_t helloWidget;
static widget_t frameWidget;
static widget_t splitterWidget;

widget_t* TestScreen;

static void drawHello(widget_t* self, framebuffer_t* buffer)
{
    if (self->dirty) {
        self->dirty = false;
        Framebuffer_Clear(self, buffer);
        Framebuffer_DrawText(self, buffer, 0, 0, &JetBrainsMono32, "Hello!");
    }
}

void TestScreen_Init(framebuffer_t* buffer)
{
    helloWidget = CustomWidget_Build(&drawHello);
    consoleWidget = ConsoleWidget_Build();
    splitterWidget = SplitterWidget_BuildVertical(&consoleWidget, &helloWidget, 128, true);
    frameWidget = FrameWidget_Build(&splitterWidget);
    TestScreen = &consoleWidget;

    TestScreen->layOut(TestScreen, 0, 0, buffer->width, buffer->height);
}

#include "layer_widget.h"
#include "custom_widget.h"
#include "keyboard/oled/oled.h"
#include "keyboard/oled/widgets/widget.h"
#include "keyboard/oled/fonts/fonts.h"
#include "keyboard/oled/framebuffer.h"
#include "keyboard/oled/oled_text_renderer.h"
#include "widget.h"
#include "legacy/layer.h"
#include "legacy/layer_switcher.h"

static bool needsUpdate = true;

static void drawLayer(widget_t* self, framebuffer_t* buffer)
{
    if (self->dirty || needsUpdate) {
        self->dirty = false;
        needsUpdate = false;
        Framebuffer_Clear(self, buffer);
        Framebuffer_DrawTextAnchored(self, buffer, AnchorType_Center, AnchorType_Center, &JetBrainsMono16, LayerNames[ActiveLayer], NULL);
    }
}

widget_t LayerWidget_Build()
{
    return CustomWidget_Build(&drawLayer);
}

void LayerWidget_Update()
{
    needsUpdate = true;
    Oled_RequestRedraw();
}

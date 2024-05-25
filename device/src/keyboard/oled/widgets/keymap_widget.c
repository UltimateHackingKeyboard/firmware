#include "keyboard/oled/widgets/keymap_widget.h"
#include "keyboard/oled/widgets/custom_widget.h"
#include "keyboard/oled/oled.h"
#include "keyboard/oled/widgets/widget.h"
#include "keyboard/oled/fonts/fonts.h"
#include "keyboard/oled/framebuffer.h"
#include "keyboard/oled/oled_text_renderer.h"
#include "widget.h"
#include "legacy/keymap.h"
#include "legacy/str_utils.h"
#include <string.h>

static bool needsUpdate = true;

static void drawKeymap(widget_t* self, framebuffer_t* buffer)
{
    if (self->dirty || needsUpdate) {
        self->dirty = false;
        needsUpdate = false;
        Framebuffer_Clear(self, buffer);
        string_segment_t text = GetKeymapName(CurrentKeymapIndex);
        Framebuffer_DrawTextAnchored(self, buffer, AnchorType_Center, AnchorType_Center, &JetBrainsMono16, text.start, text.end);
    }
}

widget_t KeymapWidget_Build()
{
    return CustomWidget_Build(&drawKeymap);
}

void KeymapWidget_Update()
{
    needsUpdate = true;
    Oled_RequestRedraw();
}

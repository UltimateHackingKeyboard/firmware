#include "console_widget.h"
#include "widget.h"
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "../fonts/fonts.h"
#include "../framebuffer.h"
#include "../oled_buffer.h"
#include "../oled_text_renderer.h"
#include "../oled.h"

static struct {
    uint16_t width;
    uint16_t height;
    bool dirty;
    uint8_t buffer[DISPLAY_USABLE_WIDTH*DISPLAY_USABLE_HEIGHT];
} consoleBuffer = { .width = DISPLAY_USABLE_WIDTH, .height = DISPLAY_USABLE_HEIGHT, .dirty = true };

framebuffer_t* ConsoleBuffer = (framebuffer_t*)&consoleBuffer;

void ConsoleWidget_LayOut(widget_t* self, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    self->x = x;
    self->y = y;
    self->w = w;
    self->h = h;
}

void ConsoleWidget_Draw(widget_t* self, framebuffer_t* buffer)
{
    Framebuffer_Copy(self, buffer, NULL, ConsoleBuffer, AnchorType_Begin, AnchorType_End);
}

widget_t ConsoleWidget_Build()
{
    return (widget_t){
        .type = WidgetType_Console,
        .layOut = &ConsoleWidget_LayOut,
        .draw = &ConsoleWidget_Draw,
    };
}

void Oled_LogConstant(const char* text)
{
    const lv_font_t* logFont = &CustomMono8;
    uint8_t line_height = logFont->line_height;
    Framebuffer_Shift(NULL, ConsoleBuffer, line_height);
    Framebuffer_DrawText(NULL, ConsoleBuffer, 0, ConsoleBuffer->height-line_height+2, logFont, text);
    OledNeedsRedraw = true;
}

void Oled_Log(const char *fmt, ...)
{
    va_list myargs;
    va_start(myargs, fmt);
    char buffer[256];
    vsprintf(buffer, fmt, myargs);
    Oled_LogConstant(buffer);
    OledNeedsRedraw = true;
}



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

#define MIN_CHAR_WIDTH 5
#define MIN_CHAR_HEIGHT 7
#define CONSOLE_BUFFER_LINE_COUNT (DISPLAY_HEIGHT/MIN_CHAR_HEIGHT+1)
#define CONSOLE_BUFFER_LINE_LENGTH (DISPLAY_WIDTH/MIN_CHAR_WIDTH+2)

static char consoleBuffer[CONSOLE_BUFFER_LINE_COUNT][CONSOLE_BUFFER_LINE_LENGTH] = {};
static uint8_t consoleBufferStart = 0;
static bool consoleBufferIsDirty = true;

void ConsoleWidget_LayOut(widget_t* self, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    self->x = x;
    self->y = y;
    self->w = w;
    self->h = h;
    self->dirty = true;
}

void ConsoleWidget_Draw(widget_t* self, framebuffer_t* buffer)
{
    if (self->dirty || consoleBufferIsDirty) {
        self->dirty = false;
        consoleBufferIsDirty = false;

        const lv_font_t* logFont = &CustomMono8;
        uint8_t line_height = logFont->line_height;

        Framebuffer_Clear(self, buffer);

        for (uint8_t line = 0; line < CONSOLE_BUFFER_LINE_COUNT; line++) {
            const char* text = &consoleBuffer[(consoleBufferStart + CONSOLE_BUFFER_LINE_COUNT - line)%CONSOLE_BUFFER_LINE_COUNT][0];

            Framebuffer_DrawText(self, buffer, 0, self->h - line_height*(line+1), logFont, text, NULL);
        }
    }
}

widget_t ConsoleWidget_Build()
{
    return (widget_t){
        .type = WidgetType_Console,
        .layOut = &ConsoleWidget_LayOut,
        .draw = &ConsoleWidget_Draw,
        .dirty = true,
    };
}

void Oled_LogConstant(const char* text)
{
    consoleBufferStart = (consoleBufferStart+1) % CONSOLE_BUFFER_LINE_COUNT;
    strncpy(&consoleBuffer[consoleBufferStart][0], text, CONSOLE_BUFFER_LINE_LENGTH);
    consoleBuffer[consoleBufferStart][CONSOLE_BUFFER_LINE_LENGTH-1] = '\0';

    consoleBufferIsDirty = true;
    Widget_Refresh(NULL);
}

void Oled_Log(const char *fmt, ...)
{
    va_list myargs;
    va_start(myargs, fmt);
    char buffer[256];
    vsprintf(buffer, fmt, myargs);
    Oled_LogConstant(buffer);
}



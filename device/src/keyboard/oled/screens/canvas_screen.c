#include "canvas_screen.h"
#include "keyboard/oled/oled_buffer.h"
#include "keyboard/oled/oled.h"
#include "keyboard/oled/widgets/widgets.h"
#include "keyboard/oled/screens/screen_manager.h"
#include <zephyr/sys/printk.h>

widget_t* CanvasScreen;

void CanvasScreen_Draw(uint16_t x, uint16_t y, uint8_t* data, uint16_t len) {
    ScreenManager_ActivateScreen(ScreenId_Canvas);
    widget_t canvas = (widget_t) {
        .x = x,
        .y = y,
        .w = MIN(DISPLAY_WIDTH-x, len),
        .h = 1,
    };
    // this is neccesary to mark the buffer region as dirty.
    Framebuffer_Clear(&canvas, OledBuffer);
    for (uint16_t i = 0; i < len; i++) {
        if (x >= DISPLAY_WIDTH) {
            printk("Usb attempting to draw outside of display range!\n");
        }
        Framebuffer_SetPixel(OledBuffer, x, y, data[i]);
        x++;
    }
    Oled_RequestRedraw();
}

void CanvasScreen_Init() {
    CanvasScreen = &CanvasWidget;
}

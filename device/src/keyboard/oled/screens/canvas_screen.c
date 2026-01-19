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

void CanvasScreen_DrawPacked(uint16_t x, uint16_t y, uint8_t* data, uint16_t pixelCount) {
    ScreenManager_ActivateScreen(ScreenId_Canvas);
    widget_t canvas = (widget_t) {
        .x = x,
        .y = y,
        .w = MIN(DISPLAY_WIDTH-x, pixelCount),
        .h = 1,
    };
    // Mark the buffer region as dirty
    Framebuffer_Clear(&canvas, OledBuffer);

    for (uint16_t i = 0; i < pixelCount; i++) {
        if (x >= DISPLAY_WIDTH) {
            printk("Usb attempting to draw outside of display range!\n");
            break;
        }
        // 2 pixels per byte: high nibble is first pixel, low nibble is second
        uint8_t packedByte = data[i / 2];
        uint8_t pixelValue = (i % 2 == 0) ? (packedByte >> 4) : (packedByte & 0x0F);
        // Scale 4-bit value to 8-bit for Framebuffer_SetPixel
        Framebuffer_SetPixel(OledBuffer, x, y, pixelValue << 4);
        x++;
    }
    Oled_RequestRedraw();
}

void CanvasScreen_Init() {
    CanvasScreen = &CanvasWidget;
}

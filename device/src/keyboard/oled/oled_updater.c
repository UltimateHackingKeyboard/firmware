#include "device.h"
#include "framebuffer.h"
#include "oled.h"
#include "oled_buffer.h"
#include "oled_display.h"
#include "screens/screen_manager.h"
#include "screens/test_screen.h"

static void OledUpdater_Callback();

static bool redrawWaiting = false;
static bool working = false;

static uint8_t currentX = 0;
static uint8_t currentY = 0;

void OledUpdater_Redraw()
{
    if (working) {
        redrawWaiting = true;
    } else {
        working = true;
        currentY = 0;
        currentX = DISPLAY_WIDTH/2;
        OledUpdater_Callback();
    }
}

static void OledUpdater_Callback()
{
}


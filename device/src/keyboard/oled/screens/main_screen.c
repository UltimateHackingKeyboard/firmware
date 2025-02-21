#include "main_screen.h"
#include "keyboard/oled/widgets/widgets.h"
#include <zephyr/kernel.h>
#include "debug.h"


static widget_t statusSplitter;
static widget_t keymapSplitter;
static widget_t debugLineAndTargetSplitter;

widget_t* MainScreen;

void MainScreen_Init()
{
#if DEBUG_MODE
    ATTR_UNUSED const uint8_t totalHeight = DISPLAY_HEIGHT - 2*DISPLAY_SHIFTING_MARGIN; //56
    ATTR_UNUSED const uint8_t statusHeight = 14;
    ATTR_UNUSED const uint8_t keymapHeight = 22;
    ATTR_UNUSED const uint8_t targetHeight = 14;
    ATTR_UNUSED const uint8_t debugLineHeight = 6;
#else
    ATTR_UNUSED const uint8_t statusHeight = 18;
    ATTR_UNUSED const uint8_t keymapHeight = 28;
    ATTR_UNUSED const uint8_t targetHeight = 20;
#endif


    debugLineAndTargetSplitter = SplitterWidget_BuildVertical(&TargetWidget, &DebugLineWidget, targetHeight, false);

    widget_t* debugLineAndTarget;

    if (DEBUG_MODE) {
        debugLineAndTarget = &debugLineAndTargetSplitter;
    } else {
        debugLineAndTarget = &TargetWidget;
    }

    keymapSplitter = SplitterWidget_BuildVertical(&KeymapLayerWidget, debugLineAndTarget, keymapHeight, false);
    statusSplitter = SplitterWidget_BuildVertical(&StatusWidget, &keymapSplitter, statusHeight, false);

    MainScreen = &statusSplitter;
}

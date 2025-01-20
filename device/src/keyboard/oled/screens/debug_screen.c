#include "debug_screen.h"
#include "keyboard/oled/widgets/widgets.h"

widget_t splitter;
widget_t* DebugScreen;

void DebugScreen_Init() {
    const uint8_t totalHeight = DISPLAY_HEIGHT - 2*DISPLAY_SHIFTING_MARGIN; //56
    const uint8_t debugLineHeight = 6;
    const uint8_t consoleHeight = totalHeight - debugLineHeight;
    splitter = SplitterWidget_BuildVertical(&ConsoleWidget, &DebugLineWidget, consoleHeight, true);
    DebugScreen = &splitter;
}

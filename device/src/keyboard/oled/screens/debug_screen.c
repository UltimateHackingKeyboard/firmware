#include "debug_screen.h"
#include "keyboard/oled/widgets/widgets.h"

widget_t* DebugScreen;

void DebugScreen_Init() {
    DebugScreen = &ConsoleWidget;
}

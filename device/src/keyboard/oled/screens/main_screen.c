#include "main_screen.h"
#include "keyboard/oled/widgets/widgets.h"


static widget_t statusSplitter;
static widget_t keymapSplitter;
static widget_t keymapLayerSplitter;

widget_t* MainScreen;

void MainScreen_Init()
{
    const uint8_t statusHeight = 18;
    const uint8_t keymapHeight = 28;

    keymapLayerSplitter = SplitterWidget_BuildHorizontal(&KeymapWidget, &LayerWidget, 128, false);
    keymapSplitter = SplitterWidget_BuildVertical(&KeymapLayerWidget, &TargetWidget, keymapHeight, false);
    statusSplitter = SplitterWidget_BuildVertical(&StatusWidget, &keymapSplitter, statusHeight, false);

    MainScreen = &statusSplitter;
}

#include "input_interceptor.h"
#include "keyboard/oled/screens/screen_manager.h"
#include "keyboard/oled/screens/pairing_screen.h"
#include "usb_report_updater.h"

static void(*recipient)(uint8_t) = NULL;

static void registerScancode(uint8_t scancode)
{
    hid_keyboard_report_t* inactiveReport = GetInactiveKeyboardReport();
    if (!KeyboardReport_ContainsScancode(inactiveReport, scancode) && recipient != NULL)
    {
        recipient(scancode);
    }
}

bool InputInterceptor_RegisterReport(hid_keyboard_report_t* activeReport)
{
    if (InteractivePairingInProgress) {
            recipient = &PairingScreen_RegisterScancode;
            KeyboardReport_ForeachScancode(activeReport, &registerScancode);
            return true;
    } else {
            return false;
    }
}

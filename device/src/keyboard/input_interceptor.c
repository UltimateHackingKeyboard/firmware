#include "input_interceptor.h"
#include "keyboard/oled/screens/screen_manager.h"
#include "keyboard/oled/screens/pairing_screen.h"

static void(*recipient)(uint8_t) = NULL;

static void registerScancode(uint8_t scancode)
{
    usb_basic_keyboard_report_t* inactiveReport = GetInactiveUsbBasicKeyboardReport();
    if (!UsbBasicKeyboard_ContainsScancode(inactiveReport, scancode) && recipient != NULL)
    {
        recipient(scancode);
    }
}

void InputInterceptor_RegisterReport(usb_basic_keyboard_report_t* activeReport)
{
    switch (ActiveScreen) {
        case ScreenId_Pairing:
            recipient = &PairingScreen_RegisterScancode;
            UsbBasicKeyboard_ForeachScancode(activeReport, &registerScancode);
            break;
        default:
            break;
    }
}

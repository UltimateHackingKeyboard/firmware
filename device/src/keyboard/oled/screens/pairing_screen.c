#include "pairing_screen.h"
#include "keyboard/oled/widgets/widgets.h"
#include "keyboard/oled/screens/screen_manager.h"
#include "keyboard/oled/oled.h"
#include "keyboard/oled/oled_buffer.h"
#include "keyboard/logger.h"
#include "legacy/lufa/HIDClassCommon.h"
#include "bt_conn.h"
#include "screen_manager.h"
#include "keyboard/logger.h"

static widget_t splitterWidget;
static widget_t questionLine;
static widget_t answerLine;

static widget_t pairingFailed;
static widget_t pairingSucceeded;

widget_t* PairingScreen;
widget_t* PairingSucceededScreen;
widget_t* PairingFailedScreen;

static uint8_t passwordCharCount = 0;
static uint8_t password[PASSWORD_LENGTH];
static char passwordTextBuffer[PASSWORD_LENGTH + PASSWORD_LENGTH - 1] = { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', };
static unsigned int correctPassword;

static void updatePasswordText()
{
    for (uint8_t i = 0; i < PASSWORD_LENGTH; i++) {
        if (i < passwordCharCount) {
            passwordTextBuffer[i*2] = '0'+password[i];
        } else {
            passwordTextBuffer[i*2] = '_';
        }
    }

    TextWidget_SetText(&answerLine, passwordTextBuffer);
    Oled_RequestRedraw();
}

unsigned int computePassword()
{
    unsigned int res = 0;
    for (uint8_t i = 0; i < PASSWORD_LENGTH; i++) {
        res = res*10 + password[i];
    }
    return res;
}

static void registerPasswordDigit(uint8_t digit)
{
    if (passwordCharCount < PASSWORD_LENGTH) {
        password[passwordCharCount++] = digit;
    }

    updatePasswordText();

    if (passwordCharCount == PASSWORD_LENGTH) {
        if (computePassword() == correctPassword) {
            num_comp_reply(1);
            ScreenManager_ActivateScreen(ScreenId_PairingSucceeded);
        } else {
            num_comp_reply(0);
            ScreenManager_ActivateScreen(ScreenId_PairingFailed);
        }
    }
}

void PairingScreen_RegisterScancode(uint8_t scancode)
{
    switch (scancode)
    {
        case HID_KEYBOARD_SC_ESCAPE:
            num_comp_reply(0);
            ScreenManager_ActivateScreen(ScreenId_PairingFailed);
            break;
        case HID_KEYBOARD_SC_DELETE:
        case HID_KEYBOARD_SC_BACKSPACE:
            passwordCharCount--;
            updatePasswordText();
            break;
        case HID_KEYBOARD_SC_1_AND_EXCLAMATION ... HID_KEYBOARD_SC_9_AND_OPENING_PARENTHESIS:
            registerPasswordDigit(scancode - HID_KEYBOARD_SC_1_AND_EXCLAMATION+1);
            break;
        case HID_KEYBOARD_SC_0_AND_CLOSING_PARENTHESIS:
            registerPasswordDigit(0);
            break;
    }
}

void PairingScreen_AskForPassword(unsigned int pass)
{
    correctPassword = pass;
    passwordCharCount = 0;
    updatePasswordText();
    ScreenManager_ActivateScreen(ScreenId_Pairing);
}

void PairingScreen_Init()
{
    questionLine = TextWidget_Build(&JetBrainsMono16, "Pairing code:");
    answerLine = TextWidget_Build(&JetBrainsMono16, passwordTextBuffer);
    splitterWidget = SplitterWidget_BuildHorizontal(&questionLine, &answerLine, (DISPLAY_HEIGHT-DISPLAY_SHIFTING_MARGIN)/2, false);
    PairingScreen = &splitterWidget;

    pairingFailed = TextWidget_Build(&JetBrainsMono16, "Pairing failed!");
    PairingFailedScreen = &pairingFailed;

    pairingSucceeded = TextWidget_Build(&JetBrainsMono16, "Pairing succeeded!");
    PairingSucceededScreen = &pairingSucceeded;
}

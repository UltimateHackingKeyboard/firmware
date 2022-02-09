#include "utils.h"
#include "key_states.h"
#include "macros.h"
#include "macro_shortcut_parser.h"
#include "led_display.h"
#include <string.h>

//this is noop at the moment, prepared for time when MAX_KEY_COUNT_PER_MODULE changes
//the purpose is to preserve current keyids
static uint16_t recodeId(uint16_t newFormat, uint16_t fromBase, uint16_t toBase)
{
    return toBase * (newFormat / fromBase) + (newFormat % fromBase);
}

uint16_t Utils_KeyStateToKeyId(key_state_t* key)
{
    if (key == NULL) {
        return 0;
    }
    uint32_t ptr1 = (uint32_t)(key_state_t*)key;
    uint32_t ptr2 = (uint32_t)(key_state_t*)&(KeyStates[0][0]);
    uint32_t res = (ptr1 - ptr2) / sizeof(key_state_t);
    return recodeId(res, MAX_KEY_COUNT_PER_MODULE, 64);
}

key_state_t* Utils_KeyIdToKeyState(uint16_t keyid)
{
    return &(((key_state_t*)KeyStates)[recodeId(keyid, 64, MAX_KEY_COUNT_PER_MODULE)]);
}

//TODO: Should probably be realized by the above KeyStateToKeyId
void Utils_DecodeId(uint16_t keyid, uint8_t* outSlotId, uint8_t* outSlotIdx)
{
    //we want to guarantee that slot ids will remain the same even if slot size
    //changes in the future, therefore hardcoded 64 is indeed correct
    *outSlotId = keyid/64;
    *outSlotIdx = keyid%64;
}

static void Utils_SetStatusScancodeCharacter(uint8_t scancode)
{
    Macros_SetStatusChar(MacroShortcutParser_ScancodeToCharacter(scancode));
}

void Utils_reportReport(usb_basic_keyboard_report_t* report)
{
    Macros_SetStatusString("Reporting ", NULL);

    UsbBasicKeyboard_ForeachScancode(report, &Utils_SetStatusScancodeCharacter);

    Macros_SetStatusString("\n", NULL);
}

void Utils_SafeStrCopy(char* target, const char* src, uint8_t max) {
    uint8_t stringlength = MIN(strlen(src)+1, (max));
    memcpy(target, src, stringlength);
    target[stringlength-1] = '\0';
}

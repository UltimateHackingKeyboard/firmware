#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_switch_keymap.h"
#include "keymap.h"
#include "test_mode.h"

void UsbCommand_SwitchKeymap(void)
{
    uint32_t keymapLength = GetUsbRxBufferUint8(1);
    char *keymapAbbrev = (char*)GenericHidInBuffer + 2;

    if (keymapLength > KEYMAP_ABBREVIATION_LENGTH) {
        SetUsbTxBufferUint8(0, UsbStatusCode_SwitchKeymap_InvalidAbbreviationLength);
    } else if (keymapLength == 1 && keymapAbbrev[0] == 1) {
        TestMode_Activate();
    }

    if (!SwitchKeymapByAbbreviation(keymapLength, keymapAbbrev)) {
        SetUsbTxBufferUint8(0, UsbStatusCode_SwitchKeymap_InvalidAbbreviation);
    }
}

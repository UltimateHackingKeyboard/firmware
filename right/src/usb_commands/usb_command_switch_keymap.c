#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_switch_keymap.h"
#include "keymap.h"

void UsbCommand_SwitchKeymap(void)
{
    uint32_t keymapLength = GetUsbRxBufferUint8(1);
    char *keymapAbbrev = (char*)GenericHidOutBuffer + 2;

    if (keymapLength > KEYMAP_ABBREVIATION_LENGTH) {
        SetUsbTxBufferUint8(0, UsbStatusCode_SwitchKeymap_InvalidAbbreviationLength);
    }

    if (!SwitchKeymapByAbbreviation(keymapLength, keymapAbbrev)) {
        SetUsbTxBufferUint8(0, UsbStatusCode_SwitchKeymap_InvalidAbbreviation);
    }
}

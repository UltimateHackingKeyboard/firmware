#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_switch_keymap.h"
#include "keymap.h"
#include "layer_stack.h"

void UsbCommand_SwitchKeymap(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    uint32_t keymapLength = GetUsbRxBufferUint8(1);
    char *keymapAbbrev = (char*)GenericHidOutBuffer + 2;

    if (keymapLength > KEYMAP_ABBREVIATION_LENGTH) {
        SetUsbTxBufferUint8(0, UsbStatusCode_SwitchKeymap_InvalidAbbreviationLength);
    }

    uint8_t res = SwitchKeymapByAbbreviation(keymapLength, keymapAbbrev);
    LayerStack_Reset();

    if (!res) {
        SetUsbTxBufferUint8(0, UsbStatusCode_SwitchKeymap_InvalidAbbreviation);
    }
}

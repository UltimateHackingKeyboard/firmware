#include "fsl_common.h"
#include "usb_commands/usb_command_apply_config.h"
#include "config_parser/config_globals.h"
#include "config_parser/parse_config.h"
#include "usb_interfaces/usb_interface_generic_hid.h"
#include "usb_protocol_handler.h"
#include "keymap.h"

void UsbCommand_ApplyConfig(void)
{
    // Validate the staging configuration.

    ParserRunDry = true;
    StagingUserConfigBuffer.offset = 0;
    GenericHidOutBuffer[0] = ParseConfig(&StagingUserConfigBuffer);
    *(uint16_t*)(GenericHidOutBuffer+1) = StagingUserConfigBuffer.offset;
    GenericHidOutBuffer[3] = 0;

    if (GenericHidOutBuffer[0] != UsbResponse_Success) {
        return;
    }

    // Make the staging configuration the current one.

    char oldKeymapAbbreviation[KEYMAP_ABBREVIATION_LENGTH];
    uint8_t oldKeymapAbbreviationLen;
    memcpy(oldKeymapAbbreviation, AllKeymaps[CurrentKeymapIndex].abbreviation, KEYMAP_ABBREVIATION_LENGTH);
    oldKeymapAbbreviationLen = AllKeymaps[CurrentKeymapIndex].abbreviationLen;

    uint8_t *temp = ValidatedUserConfigBuffer.buffer;
    ValidatedUserConfigBuffer.buffer = StagingUserConfigBuffer.buffer;
    StagingUserConfigBuffer.buffer = temp;

    ParserRunDry = false;
    ValidatedUserConfigBuffer.offset = 0;
    GenericHidOutBuffer[0] = ParseConfig(&ValidatedUserConfigBuffer);
    *(uint16_t*)(GenericHidOutBuffer+1) = ValidatedUserConfigBuffer.offset;
    GenericHidOutBuffer[3] = 1;

    if (GenericHidOutBuffer[0] != UsbResponse_Success) {
        return;
    }

    // Switch to the keymap of the updated configuration of the same name or the default keymap.

    for (uint8_t keymapId = 0; keymapId < AllKeymapsCount; keymapId++) {
        if (AllKeymaps[keymapId].abbreviationLen != oldKeymapAbbreviationLen) {
            continue;
        }
        if (memcmp(oldKeymapAbbreviation, AllKeymaps[keymapId].abbreviation, oldKeymapAbbreviationLen)) {
            continue;
        }
        SwitchKeymap(keymapId);
        return;
    }

    SwitchKeymap(DefaultKeymapIndex);
}

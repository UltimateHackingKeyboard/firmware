#include "usb_commands/usb_command_apply_config.h"
#include "config_parser/config_globals.h"
#include "config_parser/parse_config.h"
#include "peripherals/reset_button.h"
#include "usb_protocol_handler.h"
#include "keymap.h"
#include "macro_events.h"
#include "macros.h"

void updateUsbBuffer(uint8_t usbStatusCode, uint16_t parserOffset, parser_stage_t parserStage)
{
    SetUsbTxBufferUint8(0, usbStatusCode);
    SetUsbTxBufferUint16(1, parserOffset);
    SetUsbTxBufferUint8(3, parserStage);
}

void UsbCommand_ApplyConfig(void)
{
    static bool isBoot = true;

    // Validate the staging configuration.
    ParserRunDry = true;
    StagingUserConfigBuffer.offset = 0;
    uint8_t parseConfigStatus = ParseConfig(&StagingUserConfigBuffer);
    updateUsbBuffer(parseConfigStatus, StagingUserConfigBuffer.offset, ParsingStage_Validate);

    if (parseConfigStatus != UsbStatusCode_Success) {
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

    if (IsFactoryResetModeEnabled) {
        return;
    }

    ParserRunDry = false;
    ValidatedUserConfigBuffer.offset = 0;
    parseConfigStatus = ParseConfig(&ValidatedUserConfigBuffer);
    updateUsbBuffer(parseConfigStatus, ValidatedUserConfigBuffer.offset, ParsingStage_Apply);

    if (parseConfigStatus != UsbStatusCode_Success) {
        return;
    }

    Macros_ClearStatus();

    if (!isBoot) {
        Macros_ValidateAllMacros();
    }

    MacroEvent_OnInit();

    // Switch to the keymap of the updated configuration of the same name or the default keymap.
    if (SwitchKeymapByAbbreviation(oldKeymapAbbreviationLen, oldKeymapAbbreviation)) {
        return;
    }

    SwitchKeymapById(DefaultKeymapIndex);
    isBoot = false;
}

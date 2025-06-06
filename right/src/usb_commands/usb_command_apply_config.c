#include <string.h>
#include "ledmap.h"
#include "usb_commands/usb_command_apply_config.h"
#include "config_parser/config_globals.h"
#include "config_parser/parse_config.h"
#include "peripherals/reset_button.h"
#include "usb_protocol_handler.h"
#include "keymap.h"
#include "macro_events.h"
#include "macros/core.h"
#include "macros/status_buffer.h"
#include "debug.h"
#include "config_manager.h"
#include "led_manager.h"
#include "versioning.h"
#include "device.h"

#ifdef __ZEPHYR__
#include "state_sync.h"
#include "event_scheduler.h"
#endif

char oldKeymapAbbreviation[KEYMAP_ABBREVIATION_LENGTH];
uint8_t oldKeymapAbbreviationLen;

void updateUsbBuffer(uint8_t *GenericHidInBuffer, uint8_t usbStatusCode, uint16_t parserOffset, parser_stage_t parserStage)
{
    SetUsbTxBufferUint8(0, usbStatusCode);
    SetUsbTxBufferUint16(1, parserOffset);
    SetUsbTxBufferUint8(3, parserStage);
}

static void swapTheBuffers() {
    memcpy(oldKeymapAbbreviation, AllKeymaps[CurrentKeymapIndex].abbreviation, KEYMAP_ABBREVIATION_LENGTH);
    oldKeymapAbbreviationLen = AllKeymaps[CurrentKeymapIndex].abbreviationLen;

    config_buffer_t temp = ValidatedUserConfigBuffer;
    ValidatedUserConfigBuffer = StagingUserConfigBuffer;
    StagingUserConfigBuffer = temp;

    StagingUserConfigBuffer.isValid = false;
}

static uint8_t validateConfigAndSwapBuffers(uint8_t *GenericHidInBuffer) {
    // Validate the staging configuration.
    ParserRunDry = true;
    StagingUserConfigBuffer.offset = 0;
    StagingUserConfigBuffer.isValid = false;
    uint8_t parseConfigStatus = ParseConfig(&StagingUserConfigBuffer);

    if (parseConfigStatus == UsbStatusCode_Success) {
        StagingUserConfigBuffer.isValid = parseConfigStatus == UsbStatusCode_Success;
        swapTheBuffers();
    }

    if (GenericHidInBuffer) {
        updateUsbBuffer(GenericHidInBuffer, UsbStatusCode_Success, StagingUserConfigBuffer.offset, ParsingStage_Validate);
    }

    return parseConfigStatus;
}

static void setLedsWhite() {
    // Set the led test backlight mode, but don't activate the switch test mode.
    Ledmap_SetLedBacklightingMode(BacklightingMode_LightAll);
    EventVector_Set(EventVector_LedMapUpdateNeeded);
}

static bool hwConfigEmpty() {
    return HardwareConfig->majorVersion == 0 || HardwareConfig->majorVersion == 255;
}

void UsbCommand_ApplyFactory(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    EventVector_Unset(EventVector_ApplyConfig);

    DataModelVersion = userConfigVersion;

    // We may be applying factory configuration because we failed to apply User Configuration, therefore we don't want to rest the buffer.
    // Macros_ClearStatus(false);

    ConfigManager_ResetConfiguration(false);

#ifdef __ZEPHYR__
    printk(
            "Loading Factory Config: N/A (native version: %u.%u.%u., at %s / %s)\n",
            userConfigVersion.major, userConfigVersion.minor, userConfigVersion.patch,
            gitTag,
            DeviceMD5Checksums[DEVICE_ID]
          );
#endif

    if (hwConfigEmpty()) {
        setLedsWhite();
    }

#ifdef __ZEPHYR__
    StateSync_ResetConfig();
    StateSync_ResetRightLeftLink(true);
#endif

    LedManager_FullUpdate();
}
static uint8_t applyConfig(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    EventVector_Unset(EventVector_ApplyConfig);
    static bool isBoot = true;
    bool calledFromUsb = GenericHidOutBuffer != NULL;

    uint8_t parseConfigStatus;

    if (IsFactoryResetModeEnabled) {
        return UsbStatusCode_Success;
    }

    ConfigManager_ResetConfiguration(false);

    ParserRunDry = false;
    ValidatedUserConfigBuffer.offset = 0;
    parseConfigStatus = ParseConfig(&ValidatedUserConfigBuffer);

    if (parseConfigStatus != UsbStatusCode_Success) {
        Macros_ReportErrorPrintf(NULL, "UserConfig validated successfuly, but failed to parse. This wasn't supposed to happen.");
        return parseConfigStatus;
    }

    Macros_ClearStatus(calledFromUsb);

    if (!isBoot) {
        Macros_ValidateAllMacros();
    }

    MacroEvent_OnInit();

    if (hwConfigEmpty()) {
        setLedsWhite();
    }

#ifdef __ZEPHYR__
    StateSync_ResetConfig();
#endif

    // Switch to the keymap of the updated configuration of the same name or the default keymap.
    if (SwitchKeymapByAbbreviation(oldKeymapAbbreviationLen, oldKeymapAbbreviation, true)) {
        return UsbStatusCode_Success;
    }

    SwitchKeymapById(DefaultKeymapIndex, true);
    isBoot = false;

    return UsbStatusCode_Success;
}

uint8_t UsbCommand_ValidateAndApplyConfigSync(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer) {
    // used on both uhk60 and uhk80

    int err;

    err = validateConfigAndSwapBuffers(GenericHidInBuffer);

    if (err == UsbStatusCode_Success) {
        err = applyConfig(GenericHidOutBuffer, GenericHidInBuffer);
    }

    return err;
}

uint8_t UsbCommand_ApplyConfigSync(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer) {
    ASSERT(DEVICE_IS_UHK80);

    return applyConfig(GenericHidOutBuffer, GenericHidInBuffer);
}

uint8_t UsbCommand_ValidateAndApplyConfigAsync(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer) {
    ASSERT(DEVICE_IS_UHK80);

    bool calledFromUsb = GenericHidOutBuffer != NULL;
    int err = validateConfigAndSwapBuffers(GenericHidInBuffer);
    if (err == UsbStatusCode_Success) {
        EventVector_Set(EventVector_ApplyConfig);
        if (calledFromUsb) {
            Macros_ClearStatus(calledFromUsb);
        }
#ifdef __ZEPHYR__
        Main_Wake();
#endif
    }
    return err;
}


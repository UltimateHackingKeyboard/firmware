#include "attributes.h"
#include "config_parser/parse_config.h"
#include "keymap.h"
#include "logger.h"
#include "module.h"
#include "slave_drivers/is31fl3xxx_driver.h"
#include "slave_drivers/uhk_module_driver.h"
#include "slot.h"
#include "timer.h"
#include "init_clock.h"
#include "init_peripherals.h"
#include "lufa/HIDClassCommon.h"
#include "segment_display.h"
#include "usb_commands/usb_command_exec_macro_command.h"
#include "usb_composite_device.h"
#include "slave_scheduler.h"
#include "bus_pal_hardware.h"
#include "command.h"
#include "eeprom.h"
#include "right_key_matrix.h"
#include "usb_commands/usb_command_apply_config.h"
#include "peripherals/reset_button.h"
#include "config_parser/config_globals.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "usb_report_updater.h"
#include "macro_events.h"
#include "macros/shortcut_parser.h"
#include "macros/keyid_parser.h"
#include "ledmap.h"
#include "debug.h"
#include "event_scheduler.h"
#include "config_parser/config_globals.h"
#include "user_logic.h"
#include "usb_descriptors/usb_descriptor_strings.h"
#include "layouts/key_layout_60_to_universal.h"
#include "power_mode.h"
#include "usb_protocol_handler.h"
#include "event_scheduler.h"
#include "wormhole.h"
#include "trace.h"
#include "trace_reasons.h"

static bool IsEepromInitialized = false;
static bool IsConfigInitialized = false;
static bool IsHardwareConfigInitialized = false;

static void userConfigurationReadFinished(void)
{
    IsEepromInitialized = true;
}

static void hardwareConfigurationReadFinished(void)
{
    IsHardwareConfigInitialized = true;
    Ledmap_InitLedLayout();
    if (IsFactoryResetModeEnabled()) {
        HardwareConfig->signatureLength = HARDWARE_CONFIG_SIGNATURE_LENGTH;
        strncpy(HardwareConfig->signature, "FTY", HARDWARE_CONFIG_SIGNATURE_LENGTH);
    }
    EEPROM_LaunchTransfer(StorageOperation_Read, ConfigBufferId_StagingUserConfig, userConfigurationReadFinished);
}

static void initConfig()
{
    while (!IsConfigInitialized) {
        if (IsEepromInitialized) {

            if (IsFactoryResetModeEnabled() || UsbCommand_ValidateAndApplyConfigSync(NULL, NULL) != UsbStatusCode_Success) {
                UsbCommand_ApplyFactory(NULL, NULL);
            }
            ShortcutParser_initialize();
            KeyIdParser_initialize();
            Macros_Initialize();
            EventVector_Init();
            IsConfigInitialized = true;
        } else {
            __WFI();
        }
    }
}

static void sendFirstReport()
{
    bool success = false;
    while (!success) {
        UsbReportUpdateSemaphore |= 1 << USB_BASIC_KEYBOARD_INTERFACE_INDEX;
        usb_status_t status = UsbBasicKeyboardAction();
        if (status != kStatus_USB_Success) {
            UsbReportUpdateSemaphore &= ~(1 << USB_BASIC_KEYBOARD_INTERFACE_INDEX);
            __WFI();
        } else {
            success = true;
        }
    }
    while (UsbReportUpdateSemaphore) {
            __WFI();
    }
}

void CopyRightKeystateMatrix(void)
{
    KeyMatrix_ScanRow(&RightKeyMatrix);
    ++MatrixScanCounter;
    bool stateChanged = false;
    for (uint8_t keyId = 0; keyId < RIGHT_KEY_MATRIX_KEY_COUNT; keyId++) {
        uint8_t targetKeyId;

        // TODO: optimize this? This translation is quite costly :-/
        if (VERSION_AT_LEAST(DataModelVersion, 8, 2, 0)) {
            targetKeyId = KeyLayout_Uhk60_to_Universal[SlotId_RightKeyboardHalf][keyId];
        } else {
            targetKeyId = keyId;
        }

        if (KeyStates[SlotId_RightKeyboardHalf][targetKeyId].hardwareSwitchState != RightKeyMatrix.keyStates[keyId]) {
            KeyStates[SlotId_RightKeyboardHalf][targetKeyId].hardwareSwitchState = RightKeyMatrix.keyStates[keyId];
            stateChanged = true;
        }
    }
    if (stateChanged) {
        EventVector_Set(EventVector_StateMatrix);
    }
}

bool UsbReadyForTransfers(void) {
    if (UsbReportUpdateSemaphore && CurrentPowerMode > PowerMode_LastAwake) {
        if (Timer_GetElapsedTime(&UpdateUsbReports_LastUpdateTime) < USB_SEMAPHORE_TIMEOUT) {
            return false;
        } else {
            UsbReportUpdateSemaphore = 0;
        }
    }
    return true;
}

static void initUsb() {
    while (!IsHardwareConfigInitialized) {
        __WFI();
    }
    USB_SetSerialNo(HardwareConfig->uniqueId);
    InitUsb();
}

static void blinkSfjl() {
    KeyBacklightBrightness = 255;
    Ledmap_SetSfjlValues();
    uint32_t blinkStartTime = CurrentTime;
    while (CurrentTime - blinkStartTime < 50) {
        __WFI();
    }
    KeyBacklightBrightness = 0;
    Ledmap_SetBlackValues();
}

static bool keyIsSfjl(uint8_t keyId, uint8_t slotId) {
    if (slotId == SlotId_RightKeyboardHalf) {
        return keyId == 16 || keyId == 18;
    }
    if (slotId == SlotId_LeftKeyboardHalf) {
        return keyId == 17 || keyId == 15;
    }
    return false;
}

static void checkSleepMode() {
    ATTR_UNUSED bool someKeyActive = false;
    for (uint8_t slotId = 0; slotId <= SlotId_RightKeyboardHalf; slotId++) {
        bool matches = true;
        for (uint8_t keyId = 0; keyId < MAX_KEY_COUNT_PER_MODULE; keyId++) {
            someKeyActive |= KeyStates[slotId][keyId].hardwareSwitchState;
            if (KeyStates[slotId][keyId].hardwareSwitchState != keyIsSfjl(keyId, slotId)) {
                matches = false;
            }
        }
        if (matches) {
            PowerMode_ActivateMode(PowerMode_Awake, false, true, "uhk60 sfjl keys pressed");
            return;
        }
    }
    if (someKeyActive) {
        blinkSfjl();
    }
}

int main(void)
{
    Trace_Init();
    if (StateWormhole_IsOpen()) {
        if (StateWormhole.wasReboot || Trace_LooksLikeNaturalCauses()) {
            // Looks like a normal reboot or power on startup
            MacroStatusBuffer_InitNormal();
        }
        else {
            // Looks like a crash.
            StateWormhole.persistStatusBuffer = true;
            MacroStatusBuffer_Validate();
            Trace_Print(LogTarget_ErrorBuffer, "Looks like your uhk60 crashed.");
            MacroStatusBuffer_InitFromWormhole();
        }
        StateWormhole_Clean();
    } else {
        MacroStatusBuffer_InitNormal();
        StateWormhole_Clean();
        StateWormhole_Open();
    }

    InitClock();
    InitPeripherals();

    EEPROM_LaunchTransfer(StorageOperation_Read, ConfigBufferId_HardwareConfig, hardwareConfigurationReadFinished);

    if (IsBusPalOn) {
        init_hardware();
        handleUsbBusPalCommand();
    } else {
        InitSlaveScheduler();

        KeyMatrix_Init(&RightKeyMatrix);
        initUsb();

        initConfig();

        sendFirstReport();

        Trace_Printc("initialized");

        while (1) {
            Trace_Printc("{");
            CopyRightKeystateMatrix();

            if (CurrentPowerMode >= PowerMode_Lock) {
                checkSleepMode();
            }

            if (UsbReadyForTransfers() && EventScheduler_Vector & EventVector_UserLogicUpdateMask) {
                Trace('(');
                RunUserLogic();
                Trace(')');
            }
            if (EventVector_IsSet(EventVector_EventScheduler)) {
                EventScheduler_Process();
            }

            UserLogic_LastEventloopTime = CurrentTime;

            Trace_Printc("}");
            __WFI();
        }
    }
}

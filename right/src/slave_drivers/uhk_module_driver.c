#include "i2c_addresses.h"
#include "i2c.h"
#include "slave_scheduler.h"
#include "slave_drivers/uhk_module_driver.h"
#include "slave_protocol.h"
#include "main.h"
#include "peripherals/test_led.h"
#include "bool_array_converter.h"
#include "crc16.h"

uhk_module_vars_t UhkModuleVars[UHK_MODULE_MAX_COUNT];
static uhk_module_state_t uhkModuleStates[UHK_MODULE_MAX_COUNT];

static i2c_message_t rxMessage;
static i2c_message_t txMessage;

static status_t tx(void) {
    return I2cAsyncWriteMessage(I2C_ADDRESS_LEFT_KEYBOARD_HALF_FIRMWARE, &txMessage);
}

static status_t rx(void) {
    return I2cAsyncReadMessage(I2C_ADDRESS_LEFT_KEYBOARD_HALF_FIRMWARE, &rxMessage);
}

void UhkModuleSlaveDriver_Init(uint8_t uhkModuleId)
{
    uhk_module_vars_t *uhkModuleSourceVars = UhkModuleVars + uhkModuleId;
    uhk_module_state_t *uhkModuleState = uhkModuleStates + uhkModuleId;
    uhk_module_vars_t *uhkModuleTargetVars = &uhkModuleState->targetVars;

    uhkModuleSourceVars->isTestLedOn = true;
    uhkModuleTargetVars->isTestLedOn = false;

    uhkModuleSourceVars->ledPwmBrightness = MAX_PWM_BRIGHTNESS;
    uhkModuleTargetVars->ledPwmBrightness = 0;

    uhk_module_phase_t *uhkModulePhase = &uhkModuleState->phase;
    *uhkModulePhase = UhkModulePhase_RequestKeyStates;
}

status_t UhkModuleSlaveDriver_Update(uint8_t uhkModuleId)
{
    status_t status = kStatus_Uhk_IdleSlave;
    uhk_module_vars_t *uhkModuleSourceVars = UhkModuleVars + uhkModuleId;
    uhk_module_state_t *uhkModuleState = uhkModuleStates + uhkModuleId;
    uhk_module_vars_t *uhkModuleTargetVars = &uhkModuleState->targetVars;
    uhk_module_phase_t *uhkModulePhase = &uhkModuleState->phase;

    switch (*uhkModulePhase) {
        case UhkModulePhase_RequestKeyStates:
            txMessage.data[0] = SlaveCommand_RequestKeyStates;
            txMessage.length = 1;
            status = tx();
            *uhkModulePhase = UhkModulePhase_ReceiveKeystates;
            break;
        case UhkModulePhase_ReceiveKeystates:
            status = rx();
            *uhkModulePhase = UhkModulePhase_ProcessKeystates;
            break;
        case UhkModulePhase_ProcessKeystates:
            if (CRC16_IsMessageValid(&rxMessage)) {
                BoolBitsToBytes(rxMessage.data, CurrentKeyStates[SlotId_LeftKeyboardHalf], LEFT_KEYBOARD_HALF_KEY_COUNT);
            }
            status = kStatus_Uhk_NoTransfer;
            *uhkModulePhase = UhkModulePhase_SetTestLed;
            break;
        case UhkModulePhase_SetTestLed:
            if (uhkModuleSourceVars->isTestLedOn == uhkModuleTargetVars->isTestLedOn) {
                status = kStatus_Uhk_NoTransfer;
            } else {
                txMessage.data[0] = SlaveCommand_SetTestLed;
                txMessage.data[1] = uhkModuleSourceVars->isTestLedOn;
                txMessage.length = 2;
                status = tx();
                uhkModuleTargetVars->isTestLedOn = uhkModuleSourceVars->isTestLedOn;
            }
            *uhkModulePhase = UhkModulePhase_SetLedPwmBrightness;
            break;
        case UhkModulePhase_SetLedPwmBrightness:
            if (uhkModuleSourceVars->ledPwmBrightness == uhkModuleTargetVars->ledPwmBrightness) {
                status = kStatus_Uhk_NoTransfer;
            } else {
                txMessage.data[0] = SlaveCommand_SetLedPwmBrightness;
                txMessage.data[1] = uhkModuleSourceVars->ledPwmBrightness;
                txMessage.length = 2;
                status = tx();
                uhkModuleTargetVars->ledPwmBrightness = uhkModuleSourceVars->ledPwmBrightness;
            }
            *uhkModulePhase = UhkModulePhase_RequestKeyStates;
            break;
    }

    return status;
}

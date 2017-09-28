#include "i2c_addresses.h"
#include "i2c.h"
#include "slave_scheduler.h"
#include "slave_drivers/uhk_module_driver.h"
#include "slave_protocol.h"
#include "main.h"
#include "peripherals/test_led.h"
#include "bool_array_converter.h"
#include "crc16.h"

uhk_module_state_t UhkModuleStates[UHK_MODULE_MAX_COUNT];
static uhk_module_state_t uhkModuleTargetStates[UHK_MODULE_MAX_COUNT];
static uhk_module_phase_t uhkModulePhases[UHK_MODULE_MAX_COUNT];

i2c_message_t rxMessage;
i2c_message_t txMessage;

status_t tx() {
    return I2cAsyncWriteMessage(I2C_ADDRESS_LEFT_KEYBOARD_HALF_FIRMWARE, &txMessage);
}

status_t rx() {
    return I2cAsyncReadMessage(I2C_ADDRESS_LEFT_KEYBOARD_HALF_FIRMWARE, &rxMessage);
}

void UhkModuleSlaveDriver_Init(uint8_t uhkModuleId)
{
    uhk_module_state_t* uhkModuleSourceState = UhkModuleStates + uhkModuleId;
    uhk_module_state_t* uhkModuleTargetState = uhkModuleTargetStates + uhkModuleId;

    uhkModuleSourceState->isTestLedOn = true;
    uhkModuleTargetState->isTestLedOn = false;

    uhkModuleSourceState->ledPwmBrightness = MAX_PWM_BRIGHTNESS;
    uhkModuleTargetState->ledPwmBrightness = 0;

    uhk_module_phase_t *uhkModulePhase = uhkModulePhases + uhkModuleId;
    *uhkModulePhase = UhkModulePhase_RequestKeyStates;
}

status_t UhkModuleSlaveDriver_Update(uint8_t uhkModuleId)
{
    status_t status = kStatus_Uhk_IdleSlave;
    uhk_module_state_t *uhkModuleSourceState = UhkModuleStates + uhkModuleId;
    uhk_module_state_t *uhkModuleTargetState = uhkModuleTargetStates + uhkModuleId;
    uhk_module_phase_t *uhkModulePhase = uhkModulePhases + uhkModuleId;

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
            status = kStatus_Uhk_NoOp;
            *uhkModulePhase = UhkModulePhase_SetTestLed;
            break;
        case UhkModulePhase_SetTestLed:
            if (uhkModuleSourceState->isTestLedOn == uhkModuleTargetState->isTestLedOn) {
                status = kStatus_Uhk_NoOp;
            } else {
                txMessage.data[0] = SlaveCommand_SetTestLed;
                txMessage.data[1] = uhkModuleSourceState->isTestLedOn;
                txMessage.length = 2;
                status = tx();
                uhkModuleTargetState->isTestLedOn = uhkModuleSourceState->isTestLedOn;
            }
            *uhkModulePhase = UhkModulePhase_SetLedPwmBrightness;
            break;
        case UhkModulePhase_SetLedPwmBrightness:
            if (uhkModuleSourceState->ledPwmBrightness == uhkModuleTargetState->ledPwmBrightness) {
                status = kStatus_Uhk_NoOp;
            } else {
                txMessage.data[0] = SlaveCommand_SetLedPwmBrightness;
                txMessage.data[1] = uhkModuleSourceState->ledPwmBrightness;
                txMessage.length = 2;
                status = tx();
                uhkModuleTargetState->ledPwmBrightness = uhkModuleSourceState->ledPwmBrightness;
            }
            *uhkModulePhase = UhkModulePhase_RequestKeyStates;
            break;
    }

    return status;
}

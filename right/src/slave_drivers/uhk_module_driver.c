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
uhk_module_phase_t uhkModulePhase = UhkModulePhase_RequestKeyStates;

i2c_message_t rxMessage;
i2c_message_t txMessage;

void UhkModuleSlaveDriver_Init(uint8_t uhkModuleId)
{
    uhk_module_state_t* uhkModuleState = UhkModuleStates + uhkModuleId;
    uhkModuleState->isTestLedOn = true;
    uhkModuleState->ledPwmBrightness = 0x64;
}

status_t UhkModuleSlaveDriver_Update(uint8_t uhkModuleId)
{
    status_t status = kStatus_Uhk_IdleSlave;
    uhk_module_state_t *uhkModuleInternalState = UhkModuleStates + uhkModuleId;

    switch (uhkModulePhase) {
        case UhkModulePhase_RequestKeyStates:
            txMessage.data[0] = SlaveCommand_RequestKeyStates;
            txMessage.length = 1;
            status = I2cAsyncWriteMessage(I2C_ADDRESS_LEFT_KEYBOARD_HALF_FIRMWARE, &txMessage);
            uhkModulePhase = UhkModulePhase_ReceiveKeystates;
            break;
        case UhkModulePhase_ReceiveKeystates:
            status = I2cAsyncReadMessage(I2C_ADDRESS_LEFT_KEYBOARD_HALF_FIRMWARE, &rxMessage);
            uhkModulePhase = UhkModulePhase_ProcessKeystates;
            break;
        case UhkModulePhase_ProcessKeystates:
            if (CRC16_IsMessageValid(&rxMessage)) {
                BoolBitsToBytes(rxMessage.data, CurrentKeyStates[SLOT_ID_LEFT_KEYBOARD_HALF], LEFT_KEYBOARD_HALF_KEY_COUNT);
            }
            status = kStatus_Uhk_NoOp;
            uhkModulePhase = UhkModulePhase_SetTestLed;
            break;
        case UhkModulePhase_SetTestLed:
            txMessage.data[0] = SlaveCommand_SetTestLed;
            txMessage.data[1] = uhkModuleInternalState->isTestLedOn;
            txMessage.length = 2;
            status = I2cAsyncWriteMessage(I2C_ADDRESS_LEFT_KEYBOARD_HALF_FIRMWARE, &txMessage);
            uhkModulePhase = UhkModulePhase_SetLedPwmBrightness;
            break;
        case UhkModulePhase_SetLedPwmBrightness:
            txMessage.data[0] = SlaveCommand_SetLedPwmBrightness;
            txMessage.data[1] = uhkModuleInternalState->ledPwmBrightness;
            txMessage.length = 2;
            status = I2cAsyncWriteMessage(I2C_ADDRESS_LEFT_KEYBOARD_HALF_FIRMWARE, &txMessage);
            uhkModulePhase = UhkModulePhase_RequestKeyStates;
            break;
    }

    return status;
}

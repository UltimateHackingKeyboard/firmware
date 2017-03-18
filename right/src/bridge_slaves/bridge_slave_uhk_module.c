#include "i2c_addresses.h"
#include "i2c.h"
#include "bridge_slave_uhk_module.h"
#include "bridge_protocol.h"
#include "main.h"
#include "peripherials/test_led.h"

uhk_module_state_t UhkModuleStates[UHK_MODULE_MAX_COUNT];
uhk_module_field_t currentUhkModuleField = UhkModuleField_SendKeystatesRequestCommand;
uhk_module_state_t uhkModuleExternalStates[UHK_MODULE_MAX_COUNT];
uint8_t txBuffer[2];

bool BridgeSlaveUhkModuleHandler(uint8_t uhkModuleId) {
    uhk_module_state_t *uhkModuleInternalState = UhkModuleStates + uhkModuleId;
    uhk_module_state_t *uhkModuleExternalState = uhkModuleExternalStates + uhkModuleId;

    switch (currentUhkModuleField) {
        case UhkModuleField_SendKeystatesRequestCommand:
            txBuffer[0] = BridgeCommand_GetKeyStates;
            I2cAsyncWrite(I2C_ADDRESS_LEFT_KEYBOARD_HALF, txBuffer, 1);
            currentUhkModuleField = UhkModuleField_ReceiveKeystates;
            break;
        case UhkModuleField_ReceiveKeystates:
            I2cAsyncRead(I2C_ADDRESS_LEFT_KEYBOARD_HALF, CurrentKeyStates[SLOT_ID_LEFT_KEYBOARD_HALF], LEFT_KEYBOARD_HALF_KEY_COUNT);
            currentUhkModuleField = UhkModuleField_SendPwmBrightnessCommand;
            break;
        case UhkModuleField_SendPwmBrightnessCommand:
            txBuffer[0] = BridgeCommand_SetLedPwmBrightness;
            txBuffer[1] = uhkModuleInternalState->ledPwmBrightness;
            I2cAsyncWrite(I2C_ADDRESS_LEFT_KEYBOARD_HALF, txBuffer, 2);
            currentUhkModuleField = UhkModuleField_SendTestLedCommand;
            break;
        case UhkModuleField_SendTestLedCommand:
            txBuffer[0] = BridgeCommand_SetTestLed;
            txBuffer[1] = uhkModuleInternalState->isTestLedOn;
            I2cAsyncWrite(I2C_ADDRESS_LEFT_KEYBOARD_HALF, txBuffer, 2);
            currentUhkModuleField = UhkModuleField_SendKeystatesRequestCommand;
            break;
    }

    return true;
}

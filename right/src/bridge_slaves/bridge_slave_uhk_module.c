#include "i2c_addresses.h"
#include "i2c.h"
#include "bridge_slave_uhk_module.h"
#include "main.h"

bool BridgeSlaveUhkModuleHandler(uint8_t uhkModuleId) {
    I2cAsyncRead(I2C_ADDRESS_LEFT_KEYBOARD_HALF, CurrentKeyStates[SLOT_ID_LEFT_KEYBOARD_HALF], LEFT_KEYBOARD_HALF_KEY_COUNT);
    return true;
}

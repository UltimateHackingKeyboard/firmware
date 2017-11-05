#include "usb_commands/usb_command_set_test_led.h"
#include "usb_protocol_handler.h"
#include "peripherals/test_led.h"
#include "slave_drivers/uhk_module_driver.h"

void UsbCommand_SetTestLed(void)
{
    uint8_t ledState = GenericHidInBuffer[1];
    TEST_LED_SET(ledState);
    UhkModuleStates[UhkModuleDriverId_LeftKeyboardHalf].sourceVars.isTestLedOn = ledState;
}

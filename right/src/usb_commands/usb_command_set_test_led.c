#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_set_test_led.h"
#include "peripherals/test_led.h"
#include "slave_drivers/uhk_module_driver.h"

void UsbCommand_SetTestLed(void)
{
    bool isTestLedOn = GetUsbRxBufferUint8(1);
    TestLed_Set(isTestLedOn);
    UhkModuleStates[UhkModuleDriverId_LeftKeyboardHalf].sourceVars.isTestLedOn = isTestLedOn;
}

#include "fsl_common.h"
#include "usb_commands/usb_command_get_keyboard_state.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"
#include "peripherals/merge_sensor.h"
#include "slave_drivers/uhk_module_driver.h"

void UsbCommand_GetKeyboardState(void)
{
    SET_USB_BUFFER_UINT8(1, IsEepromBusy);
    SET_USB_BUFFER_UINT8(2, MERGE_SENSOR_IS_MERGED);
    SET_USB_BUFFER_UINT8(3, UhkModuleStates[UhkModuleDriverId_LeftKeyboardHalf].moduleId);
    SET_USB_BUFFER_UINT8(4, UhkModuleStates[UhkModuleDriverId_LeftAddon].moduleId);
    SET_USB_BUFFER_UINT8(5, UhkModuleStates[UhkModuleDriverId_RightAddon].moduleId);
}

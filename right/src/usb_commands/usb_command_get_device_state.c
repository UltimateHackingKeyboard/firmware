#include "fsl_common.h"
#include "usb_commands/usb_command_get_device_state.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"
#include "peripherals/merge_sensor.h"
#include "slave_drivers/uhk_module_driver.h"
#include "usb_report_updater.h"
#include "timer.h"

void UsbCommand_GetKeyboardState(void)
{
    SetUsbTxBufferUint8(1, IsEepromBusy);
    SetUsbTxBufferUint8(2, MERGE_SENSOR_IS_MERGED);
    SetUsbTxBufferUint8(3, UhkModuleStates[UhkModuleDriverId_LeftKeyboardHalf].moduleId);
    SetUsbTxBufferUint8(4, UhkModuleStates[UhkModuleDriverId_LeftModule].moduleId);
    SetUsbTxBufferUint8(5, UhkModuleStates[UhkModuleDriverId_RightModule].moduleId);
    SetUsbTxBufferUint8(6, PreviousLayer); // It's actually the active layer in practice, but with stable state.
    LastUsbGetKeyboardStateRequestTimestamp = CurrentTime;
}

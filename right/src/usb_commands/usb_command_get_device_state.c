#include "fsl_common.h"
#include "usb_commands/usb_command_get_device_state.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"
#include "peripherals/merge_sensor.h"
#include "slave_drivers/uhk_module_driver.h"
#include "usb_report_updater.h"
#include "timer.h"
#include "layer_switcher.h"
#include "slave_scheduler.h"

void UsbCommand_GetKeyboardState(void)
{
    SetUsbTxBufferUint8(1, IsEepromBusy);
    SetUsbTxBufferUint8(2, MERGE_SENSOR_IS_MERGED);
    SetUsbTxBufferUint8(3, UhkModuleStates[UhkModuleDriverId_LeftKeyboardHalf].moduleId);
    SetUsbTxBufferUint8(4, UhkModuleStates[UhkModuleDriverId_LeftModule].moduleId);
    uint8_t rightSlotModuleId = Slaves[SlaveId_RightTouchpad].isConnected
        ? ModuleId_TouchpadRight
        : UhkModuleStates[UhkModuleDriverId_RightModule].moduleId;
    SetUsbTxBufferUint8(5, rightSlotModuleId);
    SetUsbTxBufferUint8(6, ActiveLayer | (ActiveLayer != LayerId_Base && !ActiveLayerHeld ? (1 << 7) : 0) ); //Active layer + most significant bit if layer is toggled
    LastUsbGetKeyboardStateRequestTimestamp = CurrentTime;
}

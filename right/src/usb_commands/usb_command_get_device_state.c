#ifndef __ZEPHYR__
#include "fsl_common.h"
#endif

#include "keymap.h"
#include "macros/core.h"
#include "macros/status_buffer.h"
#include "segment_display.h"
#include "usb_commands/usb_command_get_device_state.h"
#include "usb_protocol_handler.h"
#include "storage.h"
#include "timer.h"
#include "layer_switcher.h"
#include "peripherals/merge_sensor.h"
#include "slave_drivers/uhk_module_driver.h"

#ifdef __ZEPHYR__
    #include "flash.h"
    #include "device_state.h"
    #include "usb_report_updater.h"
    #include "slave_scheduler.h"
#else
    #include "usb_report_updater.h"
    #include "slave_scheduler.h"
#endif

void UsbCommand_GetKeyboardState(void)
{

#ifdef __ZEPHYR__
    SetUsbTxBufferUint8(1, Flash_IsBusy());
#else
    SetUsbTxBufferUint8(1, IsStorageBusy);
#endif

#ifdef HAS_MERGE_SENSOR
    SetUsbTxBufferUint8(2, MergeSensor_IsMerged());
#endif

    SetUsbTxBufferUint8(3, ModuleConnectionStates[UhkModuleDriverId_LeftKeyboardHalf].moduleId);
    SetUsbTxBufferUint8(4, ModuleConnectionStates[UhkModuleDriverId_LeftModule].moduleId);
    SetUsbTxBufferUint8(5, ModuleConnectionStates[UhkModuleDriverId_RightModule].moduleId);
    SetUsbTxBufferUint8(6, ActiveLayer | (ActiveLayer != LayerId_Base && !ActiveLayerHeld ? (1 << 7) : 0) ); // Active layer + most significant bit if layer is toggled
    SetUsbTxBufferUint8(7, Macros_ConsumeStatusCharDirtyFlag);
    SetUsbTxBufferUint8(8, CurrentKeymapIndex);
    LastUsbGetKeyboardStateRequestTimestamp = CurrentTime;
}

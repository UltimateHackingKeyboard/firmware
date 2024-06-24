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

#ifdef __ZEPHYR__
    #include "flash.h"
    #include "device_state.h"
#else
    #include "slave_drivers/uhk_module_driver.h"
    #include "usb_report_updater.h"
    #include "slave_scheduler.h"

    #define MODULE_CONNECTION_STATE(SLOT_ID) ( \
            Timer_GetElapsedTime(&ModuleConnectionStates[SLOT_ID].lastTimeConnected) < 350 ? \
            ModuleConnectionStates[SLOT_ID].moduleId : 0 \
            )
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

#ifdef __ZEPHYR__
    SetUsbTxBufferUint8(3, DeviceState_IsDeviceConnected(DeviceId_Uhk80_Left) ? ModuleId_LeftKeyboardHalf : 0);
#else
    SetUsbTxBufferUint8(3, MODULE_CONNECTION_STATE(UhkModuleDriverId_LeftKeyboardHalf));
    SetUsbTxBufferUint8(4, MODULE_CONNECTION_STATE(UhkModuleDriverId_LeftModule));
    SetUsbTxBufferUint8(5, MODULE_CONNECTION_STATE(UhkModuleDriverId_RightModule));
#endif

    SetUsbTxBufferUint8(6, ActiveLayer | (ActiveLayer != LayerId_Base && !ActiveLayerHeld ? (1 << 7) : 0) ); // Active layer + most significant bit if layer is toggled
    SetUsbTxBufferUint8(7, Macros_ConsumeStatusCharDirtyFlag);
    SetUsbTxBufferUint8(8, CurrentKeymapIndex);
    LastUsbGetKeyboardStateRequestTimestamp = CurrentTime;
}

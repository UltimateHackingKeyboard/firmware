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
#include "device.h"

#ifdef __ZEPHYR__
    #include "flash.h"
    #include "device_state.h"
    #include "usb_report_updater.h"
    #include "slave_scheduler.h"
    #include "bt_pair.h"
    #include "bt_conn.h"
#else
    #include "usb_report_updater.h"
    #include "slave_scheduler.h"
    #define BtPair_OobPairingInProgress 0
    #define Bt_NewPairedDevice 0
#endif

void UsbCommand_GetKeyboardState(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{

#ifdef __ZEPHYR__
    SetUsbTxBufferUint8(1, Flash_IsBusy());
#else
    SetUsbTxBufferUint8(1, IsStorageBusy);
#endif

    uint8_t byte2 = 0
        | (MergeSensor_IsMerged() ? GetDeviceStateByte2_HalvesMerged : 0)
        | (BtPair_OobPairingInProgress ? GetDeviceStateByte2_PairingInProgress : 0)
        | (Bt_NewPairedDevice ? GetDeviceStateByte2_NewPairedDevice : 0);
    SetUsbTxBufferUint8(2, byte2);
    SetUsbTxBufferUint8(3, ModuleConnectionStates[UhkModuleDriverId_LeftKeyboardHalf].moduleId);
    SetUsbTxBufferUint8(4, ModuleConnectionStates[UhkModuleDriverId_LeftModule].moduleId);
    SetUsbTxBufferUint8(5, ModuleConnectionStates[UhkModuleDriverId_RightModule].moduleId);
    SetUsbTxBufferUint8(6, ActiveLayer | (ActiveLayer != LayerId_Base && !ActiveLayerHeld ? (1 << 7) : 0) ); // Active layer + most significant bit if layer is toggled
    SetUsbTxBufferUint8(7, Macros_ConsumeStatusCharDirtyFlag);
    SetUsbTxBufferUint8(8, CurrentKeymapIndex);

    LastUsbGetKeyboardStateRequestTimestamp = CurrentTime;
}

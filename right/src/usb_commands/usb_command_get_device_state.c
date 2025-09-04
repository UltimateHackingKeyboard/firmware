#include "logger.h"
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
#include "bt_defs.h"
#include "user_logic.h"
#include "trace.h"
#include "config_manager.h"

#ifdef __ZEPHYR__
    #include "flash.h"
    #include "device_state.h"
    #include "usb_report_updater.h"
    #include "slave_scheduler.h"
    #include "bt_pair.h"
    #include "bt_conn.h"
    #include "proxy_log_backend.h"
#else
    #include "usb_report_updater.h"
    #include "slave_scheduler.h"
    #define BtPair_PairingMode PairingMode_Off
    #define Bt_NewPairedDevice 0
    #define ProxyLog_HasLog 0
#endif

static void detectFreezes() {
    if (!Cfg.DevMode) {
        return;
    }

    static bool alreadyLogged = 0;
    static uint32_t lastCheckTime = 0;
    static uint8_t lastCheckCount = 0;

    if (lastCheckTime == UserLogic_LastEventloopTime) {
        lastCheckCount++;
    } else {
        lastCheckCount = 0;
        lastCheckTime = UserLogic_LastEventloopTime;
    }

    if (lastCheckCount > 30 && !alreadyLogged) {
        lastCheckCount = 0;
        alreadyLogged = true;

        Trace_Print(LogTarget_ErrorBuffer, "Looks like the firmware freezed. If that is the case, please report bellow trace to the devs:\n");
    }

    // Just trip it to make the event loop update UserLogic_LastEventloopTime if it is not frozen
    EventVector_Set(EventVector_NewMessage);
#ifdef __ZEPHYR__
    Main_Wake();
#endif
}

void UsbCommand_GetKeyboardState(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    detectFreezes();

#ifdef __ZEPHYR__
    SetUsbTxBufferUint8(1, Flash_IsBusy());
#else
    SetUsbTxBufferUint8(1, IsStorageBusy);
#endif

    uint8_t byte2 = 0
        | (MergeSensor_IsMerged() == MergeSensorState_Joined ? GetDeviceStateByte2_HalvesMerged : 0)
        | (BtPair_PairingMode == PairingMode_Oob ? GetDeviceStateByte2_PairingInProgress : 0)
        | (Bt_NewPairedDevice ? GetDeviceStateByte2_NewPairedDevice : 0)
        | (ProxyLog_HasLog ? GetDeviceStateByte2_ZephyrLog : 0);
    SetUsbTxBufferUint8(2, byte2);
    SetUsbTxBufferUint8(3, ModuleConnectionStates[UhkModuleDriverId_LeftKeyboardHalf].moduleId);
    SetUsbTxBufferUint8(4, ModuleConnectionStates[UhkModuleDriverId_LeftModule].moduleId);
    SetUsbTxBufferUint8(5, ModuleConnectionStates[UhkModuleDriverId_RightModule].moduleId);
    SetUsbTxBufferUint8(6, ActiveLayer | (ActiveLayer != LayerId_Base && !ActiveLayerHeld ? (1 << 7) : 0) ); // Active layer + most significant bit if layer is toggled
    SetUsbTxBufferUint8(7, Macros_ConsumeStatusCharDirtyFlag);
    SetUsbTxBufferUint8(8, CurrentKeymapIndex);

    LastUsbGetKeyboardStateRequestTimestamp = Timer_GetCurrentTime();
}

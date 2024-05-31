#include "module.h"
#include "slave_scheduler.h"
#include "slave_drivers/uhk_module_driver.h"
#include "config_manager.h"


module_configuration_t* GetModuleConfiguration(int8_t moduleId) {
    return Cfg.ModuleConfigurations + moduleId - ModuleId_FirstModule;
}

bool IsModuleAttached(module_id_t moduleId) {
    switch (moduleId) {
        case ModuleId_RightKeyboardHalf:
            return true;
        case ModuleId_LeftKeyboardHalf:
            return UhkModuleStates[UhkModuleDriverId_LeftKeyboardHalf].moduleId == moduleId;
        case ModuleId_KeyClusterLeft:
            return UhkModuleStates[UhkModuleDriverId_LeftModule].moduleId == moduleId;
        case ModuleId_TrackballRight:
        case ModuleId_TrackpointRight:
            return UhkModuleStates[UhkModuleDriverId_RightModule].moduleId == moduleId;
#ifndef __ZEPHYR__
        case ModuleId_TouchpadRight:
            return Slaves[SlaveId_RightTouchpad].isConnected;
#endif
        default:
            return false;
    }
}

slot_t ModuleIdToSlotId(module_id_t moduleId) {
    switch (moduleId) {
        case ModuleId_RightKeyboardHalf:
            return SlotId_RightKeyboardHalf;
        case ModuleId_LeftKeyboardHalf:
            return SlotId_LeftKeyboardHalf;
        case ModuleId_KeyClusterLeft:
            return SlotId_LeftModule;
        case ModuleId_TrackballRight:
        case ModuleId_TrackpointRight:
        case ModuleId_TouchpadRight:
            return SlotId_RightModule;
        default:
            return 0;
    }
}

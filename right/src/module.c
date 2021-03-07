#include "module.h"
#include "slave_scheduler.h"
#include "slave_drivers/uhk_module_driver.h"

module_configuration_t ModuleConfigurations[ModuleId_ModuleCount] = {
    { // ModuleId_KeyClusterLeft
        .speed = 1.0,
        .acceleration = 1.0,
        .navigationModes = {
            NavigationMode_Scroll, // Base layer
            NavigationMode_Cursor, // Mod layer
            NavigationMode_Caret, // Fn layer
            NavigationMode_Scroll, // Mouse layer
        }
    },
    { // ModuleId_TrackballRight
        .speed = 1.0, // min:0.2, opt:1.0, max:5
        .acceleration = 1.0, // min:0.1, opt:1.0, max:10.0
        .navigationModes = {
            NavigationMode_Cursor, // Base layer
            NavigationMode_Scroll, // Mod layer
            NavigationMode_Caret, // Fn layer
            NavigationMode_Cursor, // Mouse layer
        }
    },
    { // ModuleId_TrackpointRight
        .speed = 1.0, // min:0.2, opt:1.0, max:5
        .acceleration = 5.0, // min:0.1, opt:5.0, max:10.0
        .navigationModes = {
            NavigationMode_Cursor, // Base layer
            NavigationMode_Scroll, // Mod layer
            NavigationMode_Caret, // Fn layer
            NavigationMode_Cursor, // Mouse layer
        }
    },
    { // ModuleId_TouchpadRight
        .speed = 1.3, // min:0.2, opt:1.3, max:1.8
        .acceleration = 1.0, // min:0.1, opt:1.0, max:10.0
        .navigationModes = {
            NavigationMode_Cursor, // Base layer
            NavigationMode_Scroll, // Mod layer
            NavigationMode_Caret, // Fn layer
            NavigationMode_Cursor, // Mouse layer
        }
    },
    { // ModuleId_Next
        .speed = 1.0, // min:0.2, opt:1.0, max:5
        .acceleration = 5.0, // min:0.1, opt:5.0, max:10.0
        .navigationModes = {
            NavigationMode_Cursor, // Base layer
            NavigationMode_Scroll, // Mod layer
            NavigationMode_Caret, // Fn layer
            NavigationMode_Cursor, // Mouse layer
        }
    },
};

module_configuration_t* GetModuleConfiguration(int8_t moduleId) {
    return ModuleConfigurations + moduleId - ModuleId_FirstModule;
}

bool IsModuleAttached(module_id_t moduleId) {
    switch (moduleId) {
        case ModuleId_RightKeyboardHalf:
            return true;
        case ModuleId_LeftKeyboardHalf:
        case ModuleId_KeyClusterLeft:
        case ModuleId_TrackballRight:
        case ModuleId_TrackpointRight:
            return UhkModuleStates[UhkModuleDriverId_RightModule].moduleId;
        case ModuleId_TouchpadRight:
            return Slaves[SlaveId_RightTouchpad].isConnected;
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

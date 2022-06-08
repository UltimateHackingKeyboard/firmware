#include "module.h"
#include "slave_scheduler.h"
#include "slave_drivers/uhk_module_driver.h"

navigation_mode_t TouchpadPinchZoomMode = NavigationMode_Zoom;

module_configuration_t ModuleConfigurations[ModuleId_ModuleCount] = {
    { // ModuleId_KeyClusterLeft
        .speed = 0.0,
        .baseSpeed = 5.0,
        .xceleration = 0.0,
        .scrollSpeedDivisor = 5.0f,
        .caretSpeedDivisor = 5.0f,
        .pinchZoomSpeedDivisor = 5.0f,
        .axisLockSkew = 0.5f,
        .axisLockFirstTickSkew = 0.5f,
        .cursorAxisLock = false,
        .scrollAxisLock = true,
        .caretAxisLock = true,
        .swapAxes = false,
        .invertScrollDirection = false,
        .navigationModes = {
            NavigationMode_Scroll, // Base layer
            NavigationMode_Cursor, // Mod layer
            NavigationMode_Caret, // Fn layer
            NavigationMode_Scroll, // Mouse layer
        }
    },
    { // ModuleId_TrackballRight
        .speed = 0.5, // min:0.2, opt:1.0/0.5, max:5
        .baseSpeed = 0.5, // min: 0, opt:0.0/0.5, max 5
        .xceleration = 1.0, // min:0.0, opt:0.5/1.0, max:2.0
        .scrollSpeedDivisor = 8.0f,
        .caretSpeedDivisor = 16.0f,
        .pinchZoomSpeedDivisor = 4.0f,
        .axisLockSkew = 0.5f,
        .axisLockFirstTickSkew = 2.0f,
        .cursorAxisLock = false,
        .scrollAxisLock = true,
        .caretAxisLock = true,
        .swapAxes = false,
        .invertScrollDirection = false,
        .navigationModes = {
            NavigationMode_Cursor, // Base layer
            NavigationMode_Scroll, // Mod layer
            NavigationMode_Caret, // Fn layer
            NavigationMode_Cursor, // Mouse layer
        }
    },
    { // ModuleId_TrackpointRight
        .speed = 1.0, // min:0.2, opt:1.0, max:5
        .baseSpeed = 0.0, // min: 0, opt = 0.0, max 5
        .xceleration = 0.0, // min:0.0, opt:0.0, max:2.0
        .scrollSpeedDivisor = 8.0f,
        .caretSpeedDivisor = 16.0f,
        .pinchZoomSpeedDivisor = 4.0f,
        .axisLockSkew = 0.5f,
        .axisLockFirstTickSkew = 2.0f,
        .cursorAxisLock = false,
        .scrollAxisLock = true,
        .caretAxisLock = true,
        .swapAxes = false,
        .invertScrollDirection = false,
        .navigationModes = {
            NavigationMode_Cursor, // Base layer
            NavigationMode_Scroll, // Mod layer
            NavigationMode_Caret, // Fn layer
            NavigationMode_Cursor, // Mouse layer
        }
    },
    { // ModuleId_TouchpadRight
        .speed = 0.7, // min:0.2, opt:1.3/0.6, max:1.8
        .baseSpeed = 0.5, // min: 0, opt = 0.0/0.5, max 5
        .xceleration = 1.0, // min:0.0, opt:0.5/1.0, max:2.0
        .scrollSpeedDivisor = 8.0f,
        .caretSpeedDivisor = 16.0f,
        .pinchZoomSpeedDivisor = 4.0f,
        .axisLockSkew = 0.5f,
        .axisLockFirstTickSkew = 2.0f,
        .cursorAxisLock = false,
        .scrollAxisLock = true,
        .caretAxisLock = true,
        .swapAxes = false,
        .invertScrollDirection = false,
        .navigationModes = {
            NavigationMode_Cursor, // Base layer
            NavigationMode_Scroll, // Mod layer
            NavigationMode_Caret, // Fn layer
            NavigationMode_Cursor, // Mouse layer
        }
    },
    { // ModuleId_Next
        .speed = 1.0, // min:0.2, opt:1.0, max:5
        .baseSpeed = 0.5, // min: 0, opt = 0.0/0.5, max 5
        .xceleration = 5.0, // min:0.1, opt:5.0, max:10.0
        .scrollSpeedDivisor = 8.0f,
        .caretSpeedDivisor = 16.0f,
        .pinchZoomSpeedDivisor = 4.0f,
        .axisLockSkew = 0.5f,
        .axisLockFirstTickSkew = 1.0f,
        .cursorAxisLock = false,
        .scrollAxisLock = false,
        .caretAxisLock = true,
        .swapAxes = false,
        .invertScrollDirection = false,
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
            return UhkModuleStates[UhkModuleDriverId_LeftKeyboardHalf].moduleId == moduleId;
        case ModuleId_KeyClusterLeft:
            return UhkModuleStates[UhkModuleDriverId_LeftModule].moduleId == moduleId;
        case ModuleId_TrackballRight:
        case ModuleId_TrackpointRight:
            return UhkModuleStates[UhkModuleDriverId_RightModule].moduleId == moduleId;
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

#include "module.h"

module_configuration_t ModuleConfigurations[ModuleId_Count] = {
    { // ModuleId_KeyClusterLeft
        .speed = 1.0,
        .acceleration = 1.0,
        .navigationModes = {
            NavigationMode_Scroll, // Base layer
            NavigationMode_Caret, // Mod layer
            NavigationMode_Media, // Fn layer
            NavigationMode_Cursor, // Mouse layer
        }
    },
    { // ModuleId_TrackballRight
        .speed = 1.0, // min:0.2, opt:1.0, max:5
        .acceleration = 1.0, // min:0.1, opt:1.0, max:10.0
        .navigationModes = {
            NavigationMode_Cursor, // Base layer
            NavigationMode_Caret, // Mod layer
            NavigationMode_Media, // Fn layer
            NavigationMode_Scroll, // Mouse layer
        }
    },
    { // ModuleId_TrackpointRight
        .speed = 1.0, // min:0.2, opt:1.0, max:5
        .acceleration = 5.0, // min:0.1, opt:5.0, max:10.0
        .navigationModes = {
            NavigationMode_Cursor, // Base layer
            NavigationMode_Caret, // Mod layer
            NavigationMode_Media, // Fn layer
            NavigationMode_Scroll, // Mouse layer
        }
    },
    { // ModuleId_TouchpadRight
        .speed = 1.0, // min:0.2, opt:1.0, max:1.8
        .acceleration = 2.0, // min:0.1, opt:2.0, max:10.0
        .navigationModes = {
            NavigationMode_Cursor, // Base layer
            NavigationMode_Caret, // Mod layer
            NavigationMode_Media, // Fn layer
            NavigationMode_Scroll, // Mouse layer
        }
    }
};

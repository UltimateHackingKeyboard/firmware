#include "module.h"

navigation_mode_t ModuleNavigationModes[ModuleId_Count][LayerId_Count] = {
    { // ModuleId_KeyClusterLeft
        NavigationMode_Scroll, // Base layer
        NavigationMode_Caret, // Mod layer
        NavigationMode_Media, // Fn layer
        NavigationMode_Cursor, // Mouse layer
    },
    { // ModuleId_TrackballRight
        NavigationMode_Cursor, // Base layer
        NavigationMode_Caret, // Mod layer
        NavigationMode_Media, // Fn layer
        NavigationMode_Scroll, // Mouse layer
    },
    { // ModuleId_TrackpointRight
        NavigationMode_Cursor, // Base layer
        NavigationMode_Caret, // Mod layer
        NavigationMode_Media, // Fn layer
        NavigationMode_Scroll, // Mouse layer
    },
    { // ModuleId_TouchpadRight
        NavigationMode_Cursor, // Base layer
        NavigationMode_Caret, // Mod layer
        NavigationMode_Media, // Fn layer
        NavigationMode_Scroll, // Mouse layer
    }
};

#ifndef __MODULE_H__
#define __MODULE_H__

// Includes:

    #include "layer.h"
    #include "slot.h"
    #include "slave_protocol.h"

// Macros:

    #define MAX_KEY_COUNT_PER_MODULE     35

// Typedefs:

    typedef enum {
        NavigationMode_Cursor,
        NavigationMode_Scroll,
        NavigationMode_Caret,
        NavigationMode_Media,
        NavigationMode_Zoom,
        NavigationMode_ZoomPc,
        NavigationMode_ZoomMac,
        NavigationMode_None,
    } navigation_mode_t;

    typedef struct {
        // working 'cache'
        float currentSpeed; // px/ms

        // acceleration configurations
        float baseSpeed;
        float speed;
        float xceleration;

        // navigation mode configurations
        float scrollSpeedDivisor;
        float caretSpeedDivisor;
        float pinchZoomSpeedDivisor;

        float axisLockSkew;
        float axisLockFirstTickSkew;

        navigation_mode_t navigationModes[LayerId_Count];

        bool scrollAxisLock;
        bool cursorAxisLock;
        bool caretAxisLock;
        bool swapAxes;
        bool invertScrollDirection;
    } module_configuration_t;

// Variables:

    extern module_configuration_t ModuleConfigurations[ModuleId_ModuleCount];

// Functions:

    module_configuration_t* GetModuleConfiguration(int8_t moduleId);
    bool IsModuleAttached(module_id_t moduleId);
    slot_t ModuleIdToSlotId(module_id_t moduleId);

#endif

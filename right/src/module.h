#ifndef __MODULE_H__
#define __MODULE_H__

// Includes:

    #include "layer.h"
    #include "slot.h"
    #include "slave_protocol.h"

// Macros:

#ifdef __ZEPHYR__
    #define MAX_KEY_COUNT_PER_MODULE 60
#else
    #define MAX_KEY_COUNT_PER_MODULE 35
#endif

    #define MAX_BACKLIT_KEY_COUNT_PER_LEFT_MODULE 3

// Typedefs:

    typedef enum {
        /* fixed behaviour */
        NavigationMode_Cursor,
        NavigationMode_Scroll,
        NavigationMode_Zoom,
        NavigationMode_None,

        /* action_t mapped behaviour */
        NavigationMode_Caret,
        NavigationMode_Media,
        NavigationMode_ZoomPc,
        NavigationMode_ZoomMac,

        /* helpers */
        NavigationMode_Count,
        NavigationMode_RemappableFirst = NavigationMode_Caret,
        NavigationMode_RemappableLast = NavigationMode_ZoomMac,
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
        bool invertScrollDirectionX;
        bool invertScrollDirectionY;
    } module_configuration_t ;

// Variables:

// Functions:

    module_configuration_t* GetModuleConfiguration(int8_t moduleId);
    bool IsModuleAttached(module_id_t moduleId);
    slot_t ModuleIdToSlotId(module_id_t moduleId);

#endif

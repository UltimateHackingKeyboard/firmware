#ifndef __MODULE_H__
#define __MODULE_H__

// Includes:

    #include "layer.h"
    #include "slot.h"
    #include "slave_protocol.h"

// Macros:

    #define MAX_KEY_COUNT_PER_MODULE     64

// Typedefs:

    typedef enum {
        NavigationMode_Cursor,
        NavigationMode_Scroll,
        NavigationMode_Caret,
        NavigationMode_Media,
    } navigation_mode_t;

    typedef struct {
        float currentSpeed; // px/ms
        float speed;
        float acceleration;
        navigation_mode_t navigationModes[LayerId_Count];
    } module_configuration_t;

// Variables:

    extern module_configuration_t ModuleConfigurations[ModuleId_Count];

// Functions:

    module_configuration_t* GetModuleConfiguration(int8_t moduleId);

#endif

#ifndef __PARSE_MODULE_CONFIG_H__
#define __PARSE_MODULE_CONFIG_H__

// Includes:

    #include "basic_types.h"
    #include "parse_config.h"

// Typedefs:

    typedef enum {
        // module independent
        SerializedModuleProperty_Speed,
        SerializedModuleProperty_BaseSpeed,
        SerializedModuleProperty_Xceleration,
        SerializedModuleProperty_ScrollSpeedDivisor,
        SerializedModuleProperty_CaretSpeedDivisor,
        SerializedModuleProperty_AxisLockSkew,
        SerializedModuleProperty_AxisLockFirstTickSkew,
        SerializedModuleProperty_ScrollAxisLock,
        SerializedModuleProperty_CaretAxisLock,
        SerializedModuleProperty_InvertScrollDirectionY,

        // module specific

        // Keycluster
        SerializedModuleProperty_Keycluster_SwapAxes = 254,
        SerializedModuleProperty_Keycluster_InvertScrollDirectionX = 255,

        // Touchpad
        SerializedModuleProperty_Touchpad_PinchZoomSpeedDivisor = 253,
        SerializedModuleProperty_Touchpad_PinchZoomMode = 254,
        SerializedModuleProperty_Touchpad_HoldContinuationTimeout = 255,
    } serialized_module_property_t;

// Functions:

    parser_error_t ParseModuleConfiguration(config_buffer_t *buffer);

#endif

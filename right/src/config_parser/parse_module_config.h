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
        SerializedModuleProperty_CursorAxisLock,
        SerializedModuleProperty_ScrollAxisLock,
        SerializedModuleProperty_CaretAxisLock,
        SerializedModuleProperty_SwapAxes,
        SerializedModuleProperty_InvertScrollDirectionX,
        SerializedModuleProperty_InvertScrollDirectionY,

        // module specific

        // Touchpad
        SerializedModuleProperty_PinchZoomSpeedDivisor = 252,
        SerializedModuleProperty_PinchZoomMode = 253,
        SerializedModuleProperty_HoldContinuationTimeout = 254,

        // Other
        SerializedModuleProperty_InvalidStatePlaceholder = 255,
    } serialized_module_property_t;

// Functions:

    parser_error_t ParseModuleConfiguration(config_buffer_t *buffer);

#endif

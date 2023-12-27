#include "parse_module_config.h"
#include "basic_types.h"
#include "config_globals.h"
#include <stdint.h>
#include "config.h"
#include "parse_config.h"
#include "layer.h"
#include "module.h"
#include "mouse_controller.h"

static parser_error_t parseNavigationModes(config_buffer_t *buffer, module_configuration_t* moduleConfiguration)
{
    // parse navigation modes
    for (uint8_t layerId = 0; layerId < LayerId_RegularCount; layerId++) {
        uint8_t navigationModeId = ReadUInt8(buffer);

        if (navigationModeId >= NavigationMode_Count) {
            return ParserError_InvalidNavigationMode;
        }

        moduleConfiguration->navigationModes[layerId] = navigationModeId;
    }
    return ParserError_Success;
}


static parser_error_t parseProperty(config_buffer_t* buffer, module_configuration_t* moduleConfiguration, module_id_t moduleId, serialized_module_property_t propertyId)
{
    switch (propertyId) {
        case SerializedModuleProperty_Speed:
            moduleConfiguration->speed = ReadFloat(buffer);
            break;
        case SerializedModuleProperty_BaseSpeed:
            moduleConfiguration->baseSpeed = ReadFloat(buffer);
            break;
        case SerializedModuleProperty_Xceleration:
            moduleConfiguration->xceleration = ReadFloat(buffer);
            break;
        case SerializedModuleProperty_ScrollSpeedDivisor:
            moduleConfiguration->scrollSpeedDivisor = ReadFloat(buffer);
            break;
        case SerializedModuleProperty_CaretSpeedDivisor:
            moduleConfiguration->caretSpeedDivisor = ReadFloat(buffer);
            break;
        case SerializedModuleProperty_PinchZoomSpeedDivisor:
            moduleConfiguration->pinchZoomSpeedDivisor = ReadFloat(buffer);
            break;
        case SerializedModuleProperty_AxisLockSkew:
            moduleConfiguration->axisLockSkew = ReadFloat(buffer);
            break;
        case SerializedModuleProperty_AxisLockFirstTickSkew:
            moduleConfiguration->axisLockFirstTickSkew = ReadFloat(buffer);
            break;
        case SerializedModuleProperty_CursorAxisLock:
            moduleConfiguration->cursorAxisLock = ReadBool(buffer);
            break;
        case SerializedModuleProperty_ScrollAxisLock:
            moduleConfiguration->scrollAxisLock = ReadBool(buffer);
            break;
        case SerializedModuleProperty_CaretAxisLock:
            moduleConfiguration->caretAxisLock = ReadBool(buffer);
            break;
        case SerializedModuleProperty_SwapAxes:
            moduleConfiguration->swapAxes = ReadBool(buffer);
            break;
        case SerializedModuleProperty_InvertScrollDirectionX:
            moduleConfiguration->invertScrollDirectionX = ReadBool(buffer);
            break;
        case SerializedModuleProperty_InvertScrollDirectionY:
            moduleConfiguration->invertScrollDirectionY = ReadBool(buffer);
            break;
        default:
            if (moduleId == ModuleId_TouchpadRight) {
                switch (propertyId) {
                    case SerializedModuleProperty_PinchZoomSpeedDivisor:
                        moduleConfiguration->pinchZoomSpeedDivisor = ReadFloat(buffer);
                        break;
                    case SerializedModuleProperty_PinchZoomMode:
                        TouchpadPinchZoomMode = ReadUInt8(buffer);
                        break;
                    case SerializedModuleProperty_HoldContinuationTimeout:
                        HoldContinuationTimeout = ReadUInt16(buffer);
                        break;
                    default:
                        return ParserError_InvalidModuleProperty;
                }
            }
    }
    return ParserError_Success;
}

parser_error_t ParseModuleConfiguration(config_buffer_t *buffer)
{
    parser_error_t errorCode;

    uint8_t moduleId = ReadUInt8(buffer);
    module_configuration_t* moduleConfiguration = GetModuleConfiguration(moduleId);

    RETURN_ON_ERROR(
        parseNavigationModes(buffer, moduleConfiguration);
    )

    uint8_t modulePropertyCount = ReadUInt8(buffer);
    for (uint8_t i = 0; i < modulePropertyCount; i++) {
        uint8_t propertyId = ReadUInt8(buffer);

        RETURN_ON_ERROR(
            parseProperty(buffer, moduleConfiguration, moduleId, propertyId);
        )
    }

    return ParserError_Success;
}


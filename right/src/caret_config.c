#include "caret_config.h"
#include "arduino_hid/ConsumerAPI.h"
#include "arduino_hid/SystemAPI.h"
#include "macros/core.h"
#include "macros/status_buffer.h"
#include "config_manager.h"
#include "module.h"

caret_configuration_t* GetNavigationModeConfiguration(navigation_mode_t mode) {
    if (NavigationMode_RemappableFirst <= mode && mode <= NavigationMode_RemappableLast) {
        return &Cfg.NavigationModes[mode - NavigationMode_RemappableFirst];
    } else {
            Macros_ReportErrorNum("Mode referenced in invalid context. Only remappable modes are supported here:", mode, NULL);
            return NULL;
    }
}


void SetModuleCaretConfiguration(navigation_mode_t mode, caret_axis_t axis, bool positive, key_action_t action)
{
    caret_configuration_t* config = GetNavigationModeConfiguration(mode);

    key_action_t* actionSlot = positive ? &config->axisActions[axis].positiveAction : &config->axisActions[axis].negativeAction;

    *actionSlot = action;
}

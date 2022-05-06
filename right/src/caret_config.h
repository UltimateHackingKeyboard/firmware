#ifndef __CARET_CONFIG_H__
#define __CARET_CONFIG_H__

// Includes:

    #include "key_action.h"
    #include "module.h"

// Typedefs:

    typedef enum {
        CaretAxis_Horizontal = 0,
        CaretAxis_Vertical = 1,
        CaretAxis_Count = 2,
        CaretAxis_Inactive = CaretAxis_Count,
        CaretAxis_None = CaretAxis_Count,
    } caret_axis_t;

    typedef struct {
        key_action_t negativeAction;
        key_action_t positiveAction;
    } caret_dir_action_t;

    typedef struct {
        caret_dir_action_t axisActions[CaretAxis_Count];
    } caret_configuration_t;


// Functions:

    caret_configuration_t* GetModuleCaretConfiguration(int8_t moduleId, navigation_mode_t mode);
    void SetModuleCaretConfiguration(navigation_mode_t mode, caret_axis_t axis, bool positive, key_action_t action);

#endif

#ifndef __MACROS_DEBUG_COMMANDS_H__
#define __MACROS_DEBUG_COMMANDS_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "attributes.h"
    #include "macros_typedefs.h"
    #include "str_utils.h"

// Macros:

// Typedefs:

// Variables:

// Functions:

    macro_result_t Macros_ProcessStatsLayerStackCommand();
    macro_result_t Macros_ProcessStatsActiveKeysCommand();
    macro_result_t Macros_ProcessStatsPostponerStackCommand();
    macro_result_t Macros_ProcessStatsActiveMacrosCommand();
    macro_result_t Macros_ProcessDiagnoseCommand();
    macro_result_t Macros_ProcessStatsRecordKeyTimingCommand();
    macro_result_t Macros_ProcessStatsRuntimeCommand();

#endif

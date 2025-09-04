#ifndef __TRACE_REASONS_H__
#define __TRACE_REASONS_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "debug.h"
    #include "logger.h"

// Functions:

#ifndef __ZEPHYR__
    void Trace_PrintUhk60ReasonRegisters(device_id_t targetDeviceId, log_target_t targetInterface);
    bool Trace_LooksLikeNaturalCauses(void);
#endif

#endif

#endif

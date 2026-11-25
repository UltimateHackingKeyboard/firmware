#ifndef __LOCK_HINTS_H__
#define __LOCK_HINTS_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>

// Macros:

// Typedefs:

// Variables:

// Functions:

    void LockHints_ShowWakeHint(void);
    void LockHints_RestartLeftHalfGracePeriod(void);
    void LockHints_UpdateLeftHalfHint(void);
    void LockHints_OnPowerModeChanged(void);

#endif

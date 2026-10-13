#ifndef __LOCK_HINTS_H__
#define __LOCK_HINTS_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>

// Macros:

// Typedefs:

// Variables:

    // The display has to be lit up for the hint even though we are asleep.
    extern bool LockHints_WakeHintShown;

// Functions:

    void LockHints_ShowWakeHint(void);
    void LockHints_HideWakeHint(void);
    void LockHints_RestartLeftHalfGracePeriod(void);
    void LockHints_UpdateLeftHalfHint(void);
    void LockHints_OnPowerModeChanged(void);

#endif

#include "fsl_common.h"

extern uint32_t SystemCoreClock; // System clock frequency.

// How to setup clock using clock driver functions:
// 1. CLOCK_SetSimSafeDivs, to make sure core clock, bus clock, flexbus clock
//    and flash clock are in allowed range during clock mode switch.
// 2. Call CLOCK_SetMcgliteConfig to set MCG_Lite configuration.
// 3. Call CLOCK_SetSimConfig to set the clock configuration in SIM.

void InitClock(void)
{
    // Core clock: 48MHz
    // Bus clock: 24MHz
    const mcglite_config_t mcgliteConfig = {
        .outSrc = kMCGLITE_ClkSrcHirc,
        .irclkEnableMode = 0U,
        .ircs = kMCGLITE_Lirc8M,
        .fcrdiv = kMCGLITE_LircDivBy1,
        .lircDiv2 = kMCGLITE_LircDivBy1,
        .hircEnableInNotHircMode = true,
    };

    const sim_clock_config_t simConfig =
    {
        .er32kSrc = 0U, /* SIM_SOPT1[OSC32KSEL]. */
        .clkdiv1 = 0x00010000U, /* SIM_CLKDIV1. */
    };

    CLOCK_SetSimSafeDivs();
    CLOCK_SetMcgliteConfig(&mcgliteConfig);
    CLOCK_SetSimConfig(&simConfig);
    SystemCoreClock = 48000000U;
}

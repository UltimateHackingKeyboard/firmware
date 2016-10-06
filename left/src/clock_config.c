/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_common.h"
#include "fsl_smc.h"
#include "clock_config.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
/* System clock frequency. */
extern uint32_t SystemCoreClock;

/*******************************************************************************
 * Code
 ******************************************************************************/
/*
 * How to setup clock using clock driver functions:
 *
 * 1. CLOCK_SetSimSafeDivs, to make sure core clock, bus clock, flexbus clock
 *    and flash clock are in allowed range during clock mode switch.
 *
 * 2. Call CLOCK_SetMcgliteConfig to set MCG_Lite configuration.
 *
 * 3. Call CLOCK_SetSimConfig to set the clock configuration in SIM.
 */

void BOARD_BootClockVLPR(void)
{
    /*
     * Core clock: 2MHz
     * Bus clock: 1MHz
     */
    const mcglite_config_t mcgliteConfig = {
        .outSrc = kMCGLITE_ClkSrcLirc,
        .irclkEnableMode = kMCGLITE_IrclkEnable,
        .ircs = kMCGLITE_Lirc2M,
        .fcrdiv = kMCGLITE_LircDivBy1,
        .lircDiv2 = kMCGLITE_LircDivBy1,
        .hircEnableInNotHircMode = false,
    };

    const sim_clock_config_t simConfig =
    {
#if (defined(FSL_FEATURE_SIM_OPT_HAS_OSC32K_SELECTION) && FSL_FEATURE_SIM_OPT_HAS_OSC32K_SELECTION)
        .er32kSrc = 0U, /* SIM_SOPT1[OSC32KSEL]. */
#endif
        .clkdiv1 = 0x00010000U, /* SIM_CLKDIV1. */
    };

    CLOCK_SetSimSafeDivs();

    CLOCK_SetMcgliteConfig(&mcgliteConfig);

    CLOCK_SetSimConfig(&simConfig);

    SystemCoreClock = 2000000U;

    SMC_SetPowerModeProtection(SMC, kSMC_AllowPowerModeAll);
    SMC_SetPowerModeVlpr(SMC);
    while (SMC_GetPowerModeState(SMC) != kSMC_PowerStateVlpr)
    {
    }
}

void BOARD_BootClockRUN(void)
{
    /*
     * Core clock: 48MHz
     * Bus clock: 24MHz
     */
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
#if (defined(FSL_FEATURE_SIM_OPT_HAS_OSC32K_SELECTION) && FSL_FEATURE_SIM_OPT_HAS_OSC32K_SELECTION)
        .er32kSrc = 0U, /* SIM_SOPT1[OSC32KSEL]. */
#endif
        .clkdiv1 = 0x00010000U, /* SIM_CLKDIV1. */
    };

    CLOCK_SetSimSafeDivs();

    CLOCK_SetMcgliteConfig(&mcgliteConfig);

    CLOCK_SetSimConfig(&simConfig);

    SystemCoreClock = 48000000U;
}

void BOARD_InitOsc0(void)
{
    const osc_config_t oscConfig = {.freq = BOARD_XTAL0_CLK_HZ,
                                    .capLoad = 0U,
                                    .workMode = kOSC_ModeOscLowPower,
                                    .oscerConfig = {
                                        .enableMode = kOSC_ErClkEnable,
#if (defined(FSL_FEATURE_OSC_HAS_EXT_REF_CLOCK_DIVIDER) && FSL_FEATURE_OSC_HAS_EXT_REF_CLOCK_DIVIDER)
                                        .erclkDiv = 0U,
#endif
                                    }};

    CLOCK_InitOsc0(&oscConfig);

    /* Passing the XTAL0 frequency to clock driver. */
    CLOCK_SetXtal0Freq(BOARD_XTAL0_CLK_HZ);
}

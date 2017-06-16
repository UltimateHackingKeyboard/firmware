#include "fsl_smc.h"
#include "init_clock.h"
#include "fsl_rtc.h"
#include "peripherals/reset_button.h"

// How to setup clock using clock driver functions:
//
// 1. CLOCK_SetSimSafeDivs, to make sure core clock, bus clock, flexbus clock
//    and flash clock are in allowed range during clock mode switch.
//
// 2. Call CLOCK_Osc0Init to setup OSC clock, if it is used in target mode.
//
// 3. Set MCG configuration, MCG includes three parts: FLL clock, PLL clock and
//    internal reference clock(MCGIRCLK). Follow the steps to setup:
//
//    1). Call CLOCK_BootToXxxMode to set MCG to target mode.
//
//    2). If target mode is FBI/BLPI/PBI mode, the MCGIRCLK has been configured
//        correctly. For other modes, need to call CLOCK_SetInternalRefClkConfig
//        explicitly to setup MCGIRCLK.
//
//    3). Don't need to configure FLL explicitly, because if target mode is FLL
//        mode, then FLL has been configured by the function CLOCK_BootToXxxMode,
//        if the target mode is not FLL mode, the FLL is disabled.
//
//    4). If target mode is PEE/PBE/PEI/PBI mode, then the related PLL has been
//        setup by CLOCK_BootToXxxMode. In FBE/FBI/FEE/FBE mode, the PLL could
//        be enabled independently, call CLOCK_EnablePll0 explicitly in this case.
//
// 4. Call CLOCK_SetSimConfig to set the clock configuration in SIM.

#define BOARD_XTAL32K_CLK_HZ                              32768U     // Board RTC xtal frequency in Hz
#define BOARD_BOOTCLOCKRUN_CORE_CLOCK                     120000000U // Core clock frequency: 120000000Hz

#define MCG_PLL_DISABLE                                   0U  // MCGPLLCLK disabled
#define OSC_CAP0P                                         0U  // Oscillator 0pF capacitor load
#define OSC_ER_CLK_DISABLE                                0U  // Disable external reference clock
#define RTC_OSC_CAP_LOAD_12PF                        0x1800U  // RTC oscillator capacity load: 12pF
#define RTC_RTC32KCLK_PERIPHERALS_ENABLED                 1U  // RTC32KCLK to other peripherals: enabled
#define SIM_CLKOUT_SEL_FLEXBUS_CLK                        0U  // CLKOUT pin clock select: FlexBus clock
#define SIM_OSC32KSEL_RTC32KCLK_CLK                       2U  // OSC32KSEL select: RTC32KCLK clock (32.768kHz)
#define SIM_PLLFLLSEL_IRC48MCLK_CLK                       3U  // PLLFLL select: IRC48MCLK clock
#define SIM_PLLFLLSEL_MCGPLLCLK_CLK                       1U  // PLLFLL select: MCGPLLCLK clock
#define SIM_RTC_CLKOUT_SEL_RTC1HZCLK_CLK                  0U  // RTC clock output select: RTC1HzCLK clock
#define SIM_RTC_CLKOUT_SEL_RTC32KCLK_CLK                  1U  // RTC clock output select: RTC32KCLK clock (32.768kHz)
#define SIM_TRACE_CLK_SEL_CORE_SYSTEM_CLK                 1U  // Trace clock select: Core/system clock
#define SIM_USB_CLK_120000000HZ                   120000000U  // Input SIM frequency for USB: 120000000Hz
#define SIM_USB_CLK_48000000HZ                     48000000U  // Input SIM frequency for USB: 48000000Hz

extern uint32_t SystemCoreClock; // System clock frequency

// Description: This function is used to configuring RTC clock including enabling RTC oscillator.
// Param capLoad: RTC oscillator capacity load
// Param enableOutPeriph: Enable (1U)/Disable (0U) clock to peripherals
static void CLOCK_CONFIG_SetRtcClock(uint32_t capLoad, uint8_t enableOutPeriph)
{
    // Enable RTC clock gate.
    CLOCK_EnableClock(kCLOCK_Rtc0);

    if ((RTC->CR & RTC_CR_OSCE_MASK) == 0u) { // Only if the RTC oscillator is not already enabled.
        RTC_SetOscCapLoad(RTC, capLoad); // Set the specified capacitor configuration for the RTC oscillator.
        RTC->CR |= RTC_CR_OSCE_MASK; // Enable the RTC 32KHz oscillator.
    }

    // Output to other peripherals.
    if (enableOutPeriph) {
        RTC->CR &= ~RTC_CR_CLKO_MASK;
    } else {
        RTC->CR |= RTC_CR_CLKO_MASK;
    }

    CLOCK_SetXtal32Freq(BOARD_XTAL32K_CLK_HZ); // Set the XTAL32/RTC_CLKIN frequency based on board setting.

    // Set RTC_TSR if there is fault value in RTC.
    if (RTC->SR & RTC_SR_TIF_MASK) {
        RTC -> TSR = RTC -> TSR;
    }

    // Disable RTC clock gate.
    CLOCK_DisableClock(kCLOCK_Rtc0);
}

// Description: Configure FLL external reference divider (FRDIV).
// Param frdiv: The value to set FRDIV.
static void CLOCK_CONFIG_SetFllExtRefDiv(uint8_t frdiv)
{
    MCG->C1 = ((MCG->C1 & ~MCG_C1_FRDIV_MASK) | MCG_C1_FRDIV(frdiv));
}

mcg_config_t mcgConfig_BOARD_BootClockRUN =
    {
        .mcgMode = kMCG_ModePEE,                  // PEE - PLL Engaged External
        .irclkEnableMode = kMCG_IrclkEnable,      // MCGIRCLK enabled, MCGIRCLK disabled in STOP mode
        .ircs = kMCG_IrcSlow,                     // Slow internal reference clock selected
        .fcrdiv = 0x0U,                           // Fast IRC divider: divided by 1
        .frdiv = 0x0U,                            // FLL reference clock divider: divided by 32
        .drs = kMCG_DrsLow,                       // Low frequency range
        .dmx32 = kMCG_Dmx32Default,               // DCO has a default range of 25%
        .oscsel = kMCG_OscselIrc,                 // Selects 48 MHz IRC Oscillator
        .pll0Config =
            {
                .enableMode = MCG_PLL_DISABLE,    // MCGPLLCLK disabled
                .prdiv = 0xbU,                    // PLL Reference divider: divided by 12
                .vdiv = 0x6U,                     // VCO divider: multiplied by 30
            },
    };

sim_clock_config_t simConfig_BOARD_BootClockRUN =
    {
        .pllFllSel = SIM_PLLFLLSEL_MCGPLLCLK_CLK, // PLLFLL select: MCGPLLCLK clock
        .er32kSrc = SIM_OSC32KSEL_RTC32KCLK_CLK,  // OSC32KSEL select: RTC32KCLK clock (32.768kHz)
        .clkdiv1 = 0x1340000U,                    // SIM_CLKDIV1 - OUTDIV1: /1, OUTDIV2: /2, OUTDIV3: /4, OUTDIV4: /5
    };

void InitClock(void)
{
    // Set HSRUN power mode.
    SMC_SetPowerModeProtection(SMC, kSMC_AllowPowerModeAll);
    if (!RESET_BUTTON_IS_PRESSED) {
        SMC_SetPowerModeHsrun(SMC);
        while (SMC_GetPowerModeState(SMC) != kSMC_PowerStateHsrun)
        {
        }
    }
    // Set the system clock dividers in SIM to safe value.
    CLOCK_SetSimSafeDivs();
    // Configure RTC clock including enabling RTC oscillator.
    CLOCK_CONFIG_SetRtcClock(RTC_OSC_CAP_LOAD_12PF, RTC_RTC32KCLK_PERIPHERALS_ENABLED);
    // Configure the Internal Reference clock (MCGIRCLK).
    CLOCK_SetInternalRefClkConfig(mcgConfig_BOARD_BootClockRUN.irclkEnableMode,
                                  mcgConfig_BOARD_BootClockRUN.ircs,
                                  mcgConfig_BOARD_BootClockRUN.fcrdiv);
    // Configure FLL external reference divider (FRDIV).
    CLOCK_CONFIG_SetFllExtRefDiv(mcgConfig_BOARD_BootClockRUN.frdiv);
    // Set MCG to PEE mode.
    CLOCK_BootToPeeMode(mcgConfig_BOARD_BootClockRUN.oscsel,
                        kMCG_PllClkSelPll0,
                        &mcgConfig_BOARD_BootClockRUN.pll0Config);
    // Set the clock configuration in SIM module.
    CLOCK_SetSimConfig(&simConfig_BOARD_BootClockRUN);
    // Set SystemCoreClock variable.
    SystemCoreClock = BOARD_BOOTCLOCKRUN_CORE_CLOCK;
    // Set RTC_CLKOUT source.
    CLOCK_SetRtcClkOutClock(SIM_RTC_CLKOUT_SEL_RTC1HZCLK_CLK);
    // Enable USB FS clock.
    CLOCK_EnableUsbfs0Clock(kCLOCK_UsbSrcExt, SIM_USB_CLK_48000000HZ);
    // Set CLKOUT source.
    CLOCK_SetClkOutClock(SIM_CLKOUT_SEL_FLEXBUS_CLK);
    // Set debug trace clock source.
    CLOCK_SetTraceClock(SIM_TRACE_CLK_SEL_CORE_SYSTEM_CLK);
}

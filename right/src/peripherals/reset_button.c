#include <stdbool.h>
#include "reset_button.h"
#include "timer.h"
#ifndef __ZEPHYR__
#include "fsl_port.h"
#include "bootloader/wormhole.h"
#include "wormhole.h"
#include "trace.h"
#include "config_manager.h"
#include "macros/status_buffer.h"

void RESET_BUTTON_IRQ_HANDLER(void)
{
    static uint8_t count = 0;

    if (count++ > 10) {
        DisableIRQ(RESET_BUTTON_IRQ);
        Macros_ReportError("Looks like spurious factory button activation. Disabling factory button. Please report this.", NULL, NULL);
    }

    // We are getting spurious activations, so check that it is pressed for at least some 20ms straight
    for (volatile uint32_t i = 0; i < 300*1000; i++) {
    }

    if (!RESET_BUTTON_IS_PRESSED) {
        return;
    }

    StateWormhole.wasReboot = true;
    Wormhole.magicNumber = WORMHOLE_MAGIC_NUMBER;
    Wormhole.enumerationMode = EnumerationMode_NormalKeyboard;
    Trace_Printc("RstButton");
    NVIC_SystemReset();
    // unreachable
}

static bool factoryResetModeEnabled = false;

void InitResetButton(void)
{
    CLOCK_EnableClock(RESET_BUTTON_CLOCK);
    PORT_SetPinInterruptConfig(RESET_BUTTON_PORT, RESET_BUTTON_PIN, kPORT_InterruptFallingEdge);
    EnableIRQ(RESET_BUTTON_IRQ);
    PORT_SetPinConfig(RESET_BUTTON_PORT, RESET_BUTTON_PIN,
                      &(port_pin_config_t){.pullSelect=kPORT_PullUp, .mux=kPORT_MuxAsGpio});
    SDK_DelayAtLeastUs(10, SystemCoreClock);
    factoryResetModeEnabled = RESET_BUTTON_IS_PRESSED;
}

bool IsFactoryResetModeEnabled(void)
{
    return factoryResetModeEnabled;
}
#else
bool IsFactoryResetModeEnabled(void)
{
    return false;
}
#endif

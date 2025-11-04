#include <stdbool.h>
#include "reset_button.h"
#ifndef __ZEPHYR__
#include "fsl_port.h"
#include "bootloader/wormhole.h"
#include "trace.h"

void RESET_BUTTON_IRQ_HANDLER(void)
{
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

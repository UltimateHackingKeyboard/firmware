#include "fsl_gpio.h"
#include "fsl_port.h"
#include "blackberry_trackball.h"

pointer_delta_t BlackBerryTrackball_PointerDelta;

void BlackberryTrackball_Init(void)
{
    CLOCK_EnableClock(BLACKBERRY_TRACKBALL_LEFT_CLOCK);
    PORT_SetPinMux(BLACKBERRY_TRACKBALL_LEFT_PORT, BLACKBERRY_TRACKBALL_LEFT_PIN, kPORT_MuxAsGpio);
    PORT_SetPinInterruptConfig(BLACKBERRY_TRACKBALL_LEFT_PORT, BLACKBERRY_TRACKBALL_LEFT_PIN, kPORT_InterruptEitherEdge);
    NVIC_EnableIRQ(BLACKBERRY_TRACKBALL_LEFT_IRQ);
}

void BLACKBERRY_TRACKBALL_IRQ_HANDLER(void)
{
    GPIO_ClearPinsInterruptFlags(BLACKBERRY_TRACKBALL_LEFT_GPIO, 1U << BLACKBERRY_TRACKBALL_LEFT_PIN);
    BlackBerryTrackball_PointerDelta.x++;
}

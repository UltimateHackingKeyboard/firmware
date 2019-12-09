#include "fsl_gpio.h"
#include "fsl_port.h"
#include "trackball.h"
#include "test_led.h"

pointer_delta_t Trackball_PointerDelta;

void Trackball_Init(void)
{
    CLOCK_EnableClock(TRACKBALL_SHTDWN_CLOCK);
    PORT_SetPinMux(TRACKBALL_SHTDWN_PORT, TRACKBALL_SHTDWN_PIN, kPORT_MuxAsGpio);
    GPIO_WritePinOutput(TRACKBALL_SHTDWN_GPIO, TRACKBALL_SHTDWN_PIN, 0);

    CLOCK_EnableClock(TRACKBALL_NCS_CLOCK);
    PORT_SetPinMux(TRACKBALL_NCS_PORT, TRACKBALL_NCS_PIN, kPORT_MuxAsGpio);
    GPIO_WritePinOutput(TRACKBALL_NCS_GPIO, TRACKBALL_NCS_PIN, 0);
}

void Trackball_Update(void)
{
    Trackball_PointerDelta.x++;
    Trackball_PointerDelta.y++;
}

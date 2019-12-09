#include "fsl_gpio.h"
#include "fsl_port.h"
#include "trackball.h"
#include "test_led.h"

pointer_delta_t Trackball_PointerDelta;

void BlackberryTrackball_Init(void)
{
    CLOCK_EnableClock(TRACKBALL_LEFT_CLOCK);
    PORT_SetPinMux(TRACKBALL_LEFT_PORT, TRACKBALL_LEFT_PIN, kPORT_MuxAsGpio);

    CLOCK_EnableClock(TRACKBALL_RIGHT_CLOCK);
    PORT_SetPinMux(TRACKBALL_RIGHT_PORT, TRACKBALL_RIGHT_PIN, kPORT_MuxAsGpio);

    CLOCK_EnableClock(TRACKBALL_UP_CLOCK);
    PORT_SetPinMux(TRACKBALL_UP_PORT, TRACKBALL_UP_PIN, kPORT_MuxAsGpio);

    CLOCK_EnableClock(TRACKBALL_DOWN_CLOCK);
    PORT_SetPinMux(TRACKBALL_DOWN_PORT, TRACKBALL_DOWN_PIN, kPORT_MuxAsGpio);
}

bool oldLeft, oldRight, oldUp, oldDown;

void BlackberryTrackball_Update(void)
{
    uint8_t newLeft = GPIO_ReadPinInput(TRACKBALL_LEFT_GPIO, TRACKBALL_LEFT_PIN);
    if (oldLeft != newLeft) {
        Trackball_PointerDelta.x--;
        oldLeft = newLeft;
    }

    uint8_t newRight = GPIO_ReadPinInput(TRACKBALL_RIGHT_GPIO, TRACKBALL_RIGHT_PIN);
    if (oldRight != newRight) {
        Trackball_PointerDelta.x++;
        oldRight = newRight;
    }

    uint8_t newUp = GPIO_ReadPinInput(TRACKBALL_UP_GPIO, TRACKBALL_UP_PIN);
    if (oldUp != newUp) {
        Trackball_PointerDelta.y--;
        oldUp = newUp;
    }

    uint8_t newDown = GPIO_ReadPinInput(TRACKBALL_DOWN_GPIO, TRACKBALL_DOWN_PIN);
    if (oldDown != newDown) {
        Trackball_PointerDelta.y++;
        oldDown = newDown;
    }
}

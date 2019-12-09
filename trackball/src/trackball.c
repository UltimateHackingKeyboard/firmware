#include "fsl_gpio.h"
#include "fsl_port.h"
#include "trackball.h"
#include "test_led.h"

pointer_delta_t BlackBerryTrackball_PointerDelta;

void BlackberryTrackball_Init(void)
{
    CLOCK_EnableClock(BLACKBERRY_TRACKBALL_LEFT_CLOCK);
    PORT_SetPinMux(BLACKBERRY_TRACKBALL_LEFT_PORT, BLACKBERRY_TRACKBALL_LEFT_PIN, kPORT_MuxAsGpio);

    CLOCK_EnableClock(BLACKBERRY_TRACKBALL_RIGHT_CLOCK);
    PORT_SetPinMux(BLACKBERRY_TRACKBALL_RIGHT_PORT, BLACKBERRY_TRACKBALL_RIGHT_PIN, kPORT_MuxAsGpio);

    CLOCK_EnableClock(BLACKBERRY_TRACKBALL_UP_CLOCK);
    PORT_SetPinMux(BLACKBERRY_TRACKBALL_UP_PORT, BLACKBERRY_TRACKBALL_UP_PIN, kPORT_MuxAsGpio);

    CLOCK_EnableClock(BLACKBERRY_TRACKBALL_DOWN_CLOCK);
    PORT_SetPinMux(BLACKBERRY_TRACKBALL_DOWN_PORT, BLACKBERRY_TRACKBALL_DOWN_PIN, kPORT_MuxAsGpio);
}

bool oldLeft, oldRight, oldUp, oldDown;

void BlackberryTrackball_Update(void)
{
    uint8_t newLeft = GPIO_ReadPinInput(BLACKBERRY_TRACKBALL_LEFT_GPIO, BLACKBERRY_TRACKBALL_LEFT_PIN);
    if (oldLeft != newLeft) {
        BlackBerryTrackball_PointerDelta.x--;
        oldLeft = newLeft;
    }

    uint8_t newRight = GPIO_ReadPinInput(BLACKBERRY_TRACKBALL_RIGHT_GPIO, BLACKBERRY_TRACKBALL_RIGHT_PIN);
    if (oldRight != newRight) {
        BlackBerryTrackball_PointerDelta.x++;
        oldRight = newRight;
    }

    uint8_t newUp = GPIO_ReadPinInput(BLACKBERRY_TRACKBALL_UP_GPIO, BLACKBERRY_TRACKBALL_UP_PIN);
    if (oldUp != newUp) {
        BlackBerryTrackball_PointerDelta.y--;
        oldUp = newUp;
    }

    uint8_t newDown = GPIO_ReadPinInput(BLACKBERRY_TRACKBALL_DOWN_GPIO, BLACKBERRY_TRACKBALL_DOWN_PIN);
    if (oldDown != newDown) {
        BlackBerryTrackball_PointerDelta.y++;
        oldDown = newDown;
    }
}

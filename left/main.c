#include "board.h"
#include "fsl_clock_manager.h"

int main(void)
{
    gpio_input_pin_user_config_t inputPin[] =
    {
            {
                .pinName                       = kGpioSW2,
                .config.isPullEnable           = true,
                .config.pullSelect             = kPortPullUp,
                .config.isPassiveFilterEnabled = false,
                .config.interrupt              = kPortIntDisabled,
            },
            {
                .pinName                       = kGpioSW3,
                .config.isPullEnable           = true,
                .config.pullSelect             = kPortPullUp,
                .config.isPassiveFilterEnabled = false,
                .config.interrupt              = kPortIntDisabled,
            },
        {
            .pinName = GPIO_PINS_OUT_OF_RANGE,
        }
    };

    gpio_output_pin_user_config_t outputPin[] =
    {
        {
            .pinName              = kGpioLED1,
            .config.outputLogic   = 0,
            .config.slewRate      = kPortFastSlewRate,
            .config.driveStrength = kPortHighDriveStrength,
        },
        {
            .pinName              = kGpioLED3,
            .config.outputLogic   = 0,
            .config.slewRate      = kPortFastSlewRate,
            .config.driveStrength = kPortHighDriveStrength,
        },
        {
            .pinName = GPIO_PINS_OUT_OF_RANGE,
        }
    };

    CLOCK_SYS_EnablePortClock(PORTA_IDX);
    CLOCK_SYS_EnablePortClock(PORTB_IDX);

    BOARD_ClockInit();

    GPIO_DRV_Init(inputPin, outputPin);

    while (1) {
        uint8_t isSw2Pressed = GPIO_DRV_ReadPinInput(kGpioSW2);
        uint8_t isSw3Pressed = GPIO_DRV_ReadPinInput(kGpioSW3);
        GPIO_DRV_WritePinOutput(kGpioLED1, isSw2Pressed);
        GPIO_DRV_WritePinOutput(kGpioLED3, isSw3Pressed);
    }
}

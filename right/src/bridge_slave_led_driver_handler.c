#include "bridge_slave_led_driver_handler.h"
#include "led_driver.h"

#define BUFFER_SIZE (LED_DRIVER_LED_COUNT + 1)

uint8_t ledsBuffer[BUFFER_SIZE] = {FRAME_REGISTER_PWM_FIRST};

bool BridgeSlaveLedDriverHandler(uint8_t ledDriverId) {
    I2cAsyncWrite(I2C_ADDRESS_LED_DRIVER_LEFT, ledsBuffer, BUFFER_SIZE);
    return true;
}

void SetLeds(uint8_t ledBrightness)
{
    memset(ledsBuffer+1, ledBrightness, LED_DRIVER_LED_COUNT);
}

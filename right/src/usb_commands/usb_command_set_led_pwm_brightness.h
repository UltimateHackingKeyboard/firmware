#ifndef __USB_COMMAND_SET_LED_PWM_BRIGHTNESS_H__
#define __USB_COMMAND_SET_LED_PWM_BRIGHTNESS_H__

// Includes:

    #include <stdint.h>

// Functions:

    void UsbCommand_SetLedPwmBrightness(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif

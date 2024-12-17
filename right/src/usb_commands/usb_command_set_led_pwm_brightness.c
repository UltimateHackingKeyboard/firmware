#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_set_led_pwm_brightness.h"
#include "slave_drivers/uhk_module_driver.h"
#include "led_pwm.h"

void UsbCommand_SetLedPwmBrightness(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    uint8_t brightnessPercent = GetUsbRxBufferUint8(1);
    LedPwm_SetBrightness(brightnessPercent);
    UhkModuleStates[UhkModuleDriverId_LeftKeyboardHalf].sourceVars.ledPwmBrightness = brightnessPercent;
}

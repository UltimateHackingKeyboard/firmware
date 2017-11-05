#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_get_adc_value.h"
#include "peripherals/adc.h"

void UsbCommand_GetAdcValue(void)
{
    SET_USB_BUFFER_UINT32(1, ADC_Measure());
}

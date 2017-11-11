#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_get_adc_value.h"
#include "peripherals/adc.h"

void UsbCommand_GetAdcValue(void)
{
    SetUsbTxBufferUint32(1, ADC_Measure());
}

#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_get_adc_value.h"
#include "peripherals/adc.h"

void UsbCommand_GetAdcValue(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    SetUsbTxBufferUint32(1, ADC_Measure());
}

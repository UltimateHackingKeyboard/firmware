#ifndef __USB_COMMAND_GET_ADC_VALUE_H__
#define __USB_COMMAND_GET_ADC_VALUE_H__

// Includes:

    #include <stdint.h>

// Functions:

    void UsbCommand_GetAdcValue(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif

#ifndef __USB_COMMAND_PAIRING_H__
#define __USB_COMMAND_PAIRING_H__

// Includes:

    #include <stdint.h>

// Functions:

    void UsbCommand_GetPairingData(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);
    void UsbCommand_SetPairingData(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);
    void UsbCommand_PairCentral(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);
    void UsbCommand_PairPeripheral(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);
    void UsbCommand_Unpair(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);
    void UsbCommand_IsPaired(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);
    void UsbCommand_EnterPairingMode(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

// Typedefs:

#endif

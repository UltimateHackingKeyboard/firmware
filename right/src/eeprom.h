#ifndef __EEPROM_H__
#define __EEPROM_H__

// Macros:

    #define EEPROM_SIZE (32*1024)
    #define EEPROM_ADDRESS_LENGTH 2
    #define EEPROM_PAGE_SIZE 64
    #define EEPROM_BUFFER_SIZE (EEPROM_ADDRESS_LENGTH + EEPROM_PAGE_SIZE)

// Typedefs:

    typedef enum {
        EepromTransfer_ReadHardwareConfiguration,
        EepromTransfer_WriteHardwareConfiguration,
        EepromTransfer_ReadUserConfiguration,
        EepromTransfer_WriteUserConfiguration,
    } eeprom_transfer_t;

// Variables:

    extern bool IsEepromBusy;
    extern eeprom_transfer_t CurrentEepromTransfer;
    extern status_t EepromTransferStatus;

// Functions:

    extern void EEPROM_Init(void);
    extern status_t EEPROM_LaunchTransfer(eeprom_transfer_t transferType, void (*successCallback));

#endif

#include "usb_protocol_handler.h"
#include "slave_drivers/uhk_module_driver.h"

void UsbCommand_JumpToSlaveBootloader(void)
{
    uint8_t uhkModuleDriverId = GenericHidInBuffer[1];

    if (uhkModuleDriverId >= UHK_MODULE_MAX_COUNT) {
        SetUsbStatusCode(UsbStatusCode_JumpToSlaveBootloader_InvalidModuleDriverId);
        return;
    }

    UhkModuleStates[uhkModuleDriverId].jumpToBootloader = true;
}

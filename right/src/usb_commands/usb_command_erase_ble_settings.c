#ifdef __ZEPHYR__

#include "settings.h"
#include "usb_command_erase_ble_settings.h"

void UsbCommand_EraseAllSettings(void) {
    Settings_Erase();
}

#endif


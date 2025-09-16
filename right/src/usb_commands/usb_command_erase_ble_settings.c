#include "settings.h"
#include "usb_command_erase_ble_settings.h"
#include "bt_pair.h"

void UsbCommand_EraseAllSettings(void) {
    BtPair_UnpairAllNonLR();
    Settings_Erase("Erase usb command received.");
}

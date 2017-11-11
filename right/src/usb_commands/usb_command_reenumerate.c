#include "fsl_common.h"
#include "usb_commands/usb_command_reenumerate.h"
#include "usb_protocol_handler.h"
#include "bootloader/wormhole.h"

void UsbCommand_Reenumerate(void)
{
    Wormhole.magicNumber = WORMHOLE_MAGIC_NUMBER;
    Wormhole.enumerationMode = GetUsbRxBufferUint8(1);
    Wormhole.timeoutMs       = GetUsbRxBufferUint32(2);
    NVIC_SystemReset();
}

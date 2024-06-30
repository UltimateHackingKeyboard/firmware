#ifdef __ZEPHYR__
#include <zephyr/retention/bootmode.h>
#include <zephyr/sys/reboot.h>
#else
#include "fsl_common.h"
#include "bootloader/wormhole.h"
#endif

#include "usb_commands/usb_command_reenumerate.h"
#include "usb_protocol_handler.h"

void UsbCommand_Reenumerate(void)
{
#ifdef __ZEPHYR__
    bootmode_set(BOOT_MODE_TYPE_BOOTLOADER);
    sys_reboot(SYS_REBOOT_WARM);
#else
    Wormhole.magicNumber = WORMHOLE_MAGIC_NUMBER;
    Wormhole.enumerationMode = GetUsbRxBufferUint8(1);
    Wormhole.timeoutMs       = GetUsbRxBufferUint32(2);
    NVIC_SystemReset();
#endif
}

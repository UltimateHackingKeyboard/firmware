#ifdef __ZEPHYR__
#include <zephyr/retention/bootmode.h>
#include <zephyr/sys/reboot.h>
#include "device.h"
#include "messenger.h"
#include "connections.h"
#include "bt_conn.h"
#else
#include "fsl_common.h"
#include "bootloader/wormhole.h"
#endif

#include "usb_commands/usb_command_reenumerate.h"
#include "usb_protocol_handler.h"

void Reboot(bool rebootPeripherals) {
#ifdef __ZEPHYR__
    if (rebootPeripherals) {
        if (DEVICE_IS_UHK80_RIGHT) {
            Messenger_Send2(DeviceId_Uhk80_Left, MessageId_Command, MessengerCommand_Reboot, NULL, 0);
            for (uint8_t peerId = PeerIdFirstHost; peerId <= PeerIdLastHost; peerId++) {
                uint8_t connectionId = Peers[peerId].connectionId;
                if (Connections_Type(connectionId) == ConnectionType_NusDongle) {
                    Messenger_Send2Via(DeviceId_Uhk_Dongle, connectionId, MessageId_Command, MessengerCommand_Reboot, NULL, 0);
                }
            }
        }
        if (DEVICE_IS_UHK80_LEFT) {
            Messenger_Send2(DeviceId_Uhk80_Right, MessageId_Command, MessengerCommand_Reboot, NULL, 0);
        }
        if (DEVICE_IS_UHK_DONGLE) {
            Messenger_Send2(DeviceId_Uhk80_Right, MessageId_Command, MessengerCommand_Reboot, NULL, 0);
        }

        k_sleep(K_SECONDS(1));
    }

    sys_reboot(SYS_REBOOT_COLD);
#else
    NVIC_SystemReset();
#endif
}

void UsbCommand_Reenumerate(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
#ifdef __ZEPHYR__
    bootmode_set(BOOT_MODE_TYPE_BOOTLOADER);
    sys_reboot(SYS_REBOOT_COLD);
#else
    Wormhole.magicNumber = WORMHOLE_MAGIC_NUMBER;
    Wormhole.enumerationMode = GetUsbRxBufferUint8(1);
    Wormhole.timeoutMs       = GetUsbRxBufferUint32(2);
    NVIC_SystemReset();
#endif
}

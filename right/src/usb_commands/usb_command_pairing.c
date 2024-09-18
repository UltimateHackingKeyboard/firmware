#include "usb_command_pairing.h"
#include "usb_protocol_handler.h"
#include "device.h"

#ifdef __ZEPHYR__
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/addr.h>
#include "bt_pair.h"
#include "bt_conn.h"

#define BUF_PEER_POS 1
#define BUF_ADR_POS 1
#define BUF_KEY_R_POS 7
#define BUF_KEY_C_POS 23

void UsbCommand_GetPairingData(void) {
    struct bt_le_oob* oob = BtPair_GetLocalOob();

    SetUsbTxBufferBleAddress(BUF_ADR_POS, &oob->addr);
    memcpy(GenericHidInBuffer + BUF_KEY_R_POS, oob->le_sc_data.r, BLE_KEY_LEN);
    memcpy(GenericHidInBuffer + BUF_KEY_C_POS, oob->le_sc_data.c, BLE_KEY_LEN);
    // Print content of the hid in buffer

    for (uint8_t i = 0; i < 16; i++) {
        printk("%x ", GenericHidInBuffer[i]);
    }
    printk("\n");
}

void UsbCommand_SetPairingData(void) {
    struct bt_le_oob* oob = BtPair_GetRemoteOob();
    uint8_t peerId = GenericHidOutBuffer[BUF_PEER_POS];

    oob->addr = GetUsbRxBufferBleAddress(1 + BUF_ADR_POS);
    memcpy(oob->addr.a.val, GenericHidOutBuffer + 1 + BUF_ADR_POS, BLE_ADDR_LEN);
    memcpy(oob->le_sc_data.r, GenericHidOutBuffer + 1 + BUF_KEY_R_POS, BLE_KEY_LEN);
    memcpy(oob->le_sc_data.c, GenericHidOutBuffer + 1 + BUF_KEY_C_POS, BLE_KEY_LEN);

    //copy addres backwards to fix  endianity
    uint8_t addr[BLE_ADDR_LEN];
    for (uint8_t i = 0; i < BLE_ADDR_LEN; i++) {
        addr[i] = oob->addr.a.val[BLE_ADDR_LEN - i - 1];
    }

    switch (peerId) {
        case PeerIdLeft:
            settings_save_one("uhk/addr/left", addr, BLE_ADDR_LEN);
            break;
        case PeerIdRight:
            settings_save_one("uhk/addr/right", addr, BLE_ADDR_LEN);
            break;
        case PeerIdDongle:
            settings_save_one("uhk/addr/dongle", addr, BLE_ADDR_LEN);
            break;
        default:
    }
}

void UsbCommand_PairCentral(void) {
    BtPair_PairCentral();
}

void UsbCommand_PairPeripheral(void) {
    BtPair_PairPeripheral();
}

// If zero address is provided, all existing bonds will be deleted
void UsbCommand_Unpair(void) {
    bt_addr_le_t addr = GetUsbRxBufferBleAddress(1);
    BtPair_Unpair(addr);
}

void UsbCommand_IsPaired(void) {
    bt_addr_le_t addr = GetUsbRxBufferBleAddress(1);
    bool isPaired = BtPair_IsDeviceBonded(&addr);
    SetUsbTxBufferUint8(1, isPaired);
}

#endif

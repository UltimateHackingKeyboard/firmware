#include "stubs.h"

#define ATTRS __attribute__((weak))

    ATTRS bool SegmentDisplay_NeedsUpdate = false;
    ATTRS bool RunningOnBattery = false;
    ATTRS bool BatteryIsCharging = false;
    ATTRS bool RightRunningOnBattery = false;
    ATTRS void Oled_UpdateBrightness() {};
    ATTRS void Oled_ShiftScreen() {};
    ATTRS void ScreenManager_SwitchScreenEvent() {};
    ATTRS void Charger_UpdateBatteryState() {};
    ATTRS void Charger_UpdateBatteryCharging() {};
    ATTRS const rgb_t* PairingScreen_ActionColor(key_action_t* action) { return NULL; };
    ATTRS void Uart_Reenable() {};
    ATTRS void UartBridge_Enable() {};
    ATTRS status_t ZephyrI2c_MasterTransferNonBlocking(i2c_master_transfer_t *transfer) { return kStatus_Fail; };
    ATTRS void Oled_LogConstant(const char* text) {};
    ATTRS void Oled_Log(const char *fmt, ...) {};
    ATTRS void Uart_LogConstant(const char* buffer) {};
    ATTRS void Uart_Log(const char *fmt, ...) {};
    ATTRS void Log(const char *fmt, ...) {};
    ATTRS void LogBt(const char *fmt, ...) {};
    ATTRS void BtPair_EndPairing(bool success, const char* msg) {};
    ATTRS void BtManager_RestartBt() {};
    ATTRS void DongleLeds_Update(void) {};
    ATTRS void BtPair_ClearUnknownBonds() {};
    ATTRS void BtManager_StartScanningAndAdvertising() {};
    ATTRS void BtConn_UpdateHostConnectionPeerAllocations() {};
    ATTRS void Oled_RequestRedraw() {};
    ATTRS void RoundTripTest_Run() {};
    ATTRS void Resend_RequestResendSync() {};
    ATTRS void PairingScreen_Feedback(bool success) {};
    ATTRS void StateSync_CheckFirmwareVersions() {};
    ATTRS void StateSync_CheckDongleProtocolVersion() {};
    ATTRS void Trace(char a) {};
    ATTRS void Trace_Printc(const char* s) {};
    ATTRS void Trace_Printf(const char *fmt, ...) {};
    ATTRS void PowerMode_PutBackToSleepMaybe(void) {};
    ATTRS void BtAdvertise_DisableAdvertisingIcon(void) {};
    ATTRS void NotificationScreen_NotifyFor(const char* message, uint16_t duration) {};

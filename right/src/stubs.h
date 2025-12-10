#ifndef __STUBS_H__
#define __STUBS_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>
    #include <stddef.h>
    #include "attributes.h"
    #include "key_action.h"


    #ifdef __ZEPHYR__
        #include "keyboard/i2c_compatibility.h"
    #else
        #include "fsl_i2c.h"
    #endif

// Macros:

// Variables:

// Functions:

    extern bool SegmentDisplay_NeedsUpdate;
    extern bool RunningOnBattery;
    extern bool BatteryIsCharging;
    extern bool RightRunningOnBattery;
    extern void Oled_UpdateBrightness();
    extern void Oled_ShiftScreen();
    extern void ScreenManager_SwitchScreenEvent();
    extern void Charger_UpdateBatteryState();
    extern void Charger_UpdateBatteryCharging();
    extern const rgb_t* PairingScreen_ActionColor(key_action_t* action);
    extern void Uart_Reenable();
    extern void UartBridge_Enable();
    extern status_t ZephyrI2c_MasterTransferNonBlocking(i2c_master_transfer_t *transfer);
    extern void Oled_LogConstant(const char* text);
    extern void Oled_Log(const char *fmt, ...);
    extern void Uart_LogConstant(const char* buffer);
    extern void Uart_Log(const char *fmt, ...);
    extern void Log(const char *fmt, ...);
    extern void LogBt(const char *fmt, ...);
    extern void BtPair_EndPairing(bool success, const char* msg);
    extern void BtManager_RestartBt();
    extern void DongleLeds_Update(void);
    extern void BtPair_ClearUnknownBonds();
    extern void BtManager_StartScanningAndAdvertising();
    extern void BtConn_UpdateHostConnectionPeerAllocations();
    extern void Oled_RequestRedraw();
    extern void RoundTripTest_Run();
    extern void Resend_RequestResendSync();
    extern void PairingScreen_Feedback(bool success);
    extern void StateSync_CheckFirmwareVersions();
    extern void StateSync_CheckDongleProtocolVersion();
    extern void Trace(char a);
    extern void Trace_Printc(const char *s);
    extern void Trace_Printf(const char *fmt, ...);
    extern void PowerMode_PutBackToSleepMaybe(void);
    extern void BtAdvertise_DisableAdvertisingIcon(void);
    extern void NotificationScreen_NotifyFor(const char* message, uint16_t duration);

#if DEVICE_HAS_OLED
#define WIDGET_REFRESH(W) Widget_Refresh(W)
#else
#define WIDGET_REFRESH(W)
#endif

#endif

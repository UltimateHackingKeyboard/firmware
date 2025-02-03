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
    extern bool RightRunningOnBattery;
    extern void Oled_UpdateBrightness();
    extern void Oled_ShiftScreen();
    extern void ScreenManager_SwitchScreenEvent();
    extern void Charger_UpdateBatteryState();
    extern const rgb_t* PairingScreen_ActionColor(key_action_t* action);
    extern void Uart_Reenable();
    extern void Uart_Enable();
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
    extern uint8_t BtAdvertise_Start(uint8_t adv_type);
    extern uint8_t BtAdvertise_Type();
    extern int BtScan_Start(void);
    extern void BtManager_StartScanningAndAdvertising();
    extern void BtConn_UpdateHostConnectionPeerAllocations();
    extern void Oled_RequestRedraw();
    extern void RoundTripTest_Run();
    extern void Resend_RequestResendSync();
    extern void PairingScreen_Feedback(bool success);

#if DEVICE_HAS_OLED
#define WIDGET_REFRESH(W) Widget_Refresh(W)
#else
#define WIDGET_REFRESH(W)
#endif

#endif

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

#define ATTRS __attribute__((weak)) __attribute__((unused))

// Variables:

// Functions:

    ATTRS bool SegmentDisplay_NeedsUpdate = false;
    ATTRS bool RunningOnBattery = false;
    ATTRS bool RightRunningOnBattery = false;
    ATTRS void Oled_UpdateBrightness() {};
    ATTRS void Oled_ShiftScreen() {};
    ATTRS void ScreenManager_SwitchScreenEvent() {};
    ATTRS void Charger_UpdateBatteryState() {};
    ATTRS const rgb_t* PairingScreen_ActionColor(key_action_t* action) { return NULL; };
    ATTRS void Uart_Reenable() {};
    ATTRS void Uart_Enable() {};
    ATTRS status_t ZephyrI2c_MasterTransferNonBlocking(i2c_master_transfer_t *transfer) { return kStatus_Fail; };
    ATTRS void Oled_LogConstant(const char* text) {};
    ATTRS void Oled_Log(const char *fmt, ...) {};
    ATTRS void Uart_LogConstant(const char* buffer) {};
    ATTRS void Uart_Log(const char *fmt, ...) {};
    ATTRS void Log(const char *fmt, ...) {};
    ATTRS void LogBt(const char *fmt, ...) {};
    ATTRS void BtPair_EndPairing(bool success, const char* msg) {};
    ATTRS void BtManager_RestartBt() {};
    ATTRS void DongleLeds_Update() {};
    ATTRS void BtPair_ClearUnknownBonds() {};

#if DEVICE_HAS_OLED
#define WIDGET_REFRESH(W) Widget_Refresh(W)
#else
#define WIDGET_REFRESH(W)
#endif

#endif

#include "trace_reasons.h"

#ifndef __ZEPHYR__

// RCM (Reset Control Module) registers - K22P121M120SF7
#define RCM_SRS0    (*(volatile uint8_t*)0x4007F000)
#define RCM_SRS1    (*(volatile uint8_t*)0x4007F001)
#define RCM_SSRS0   (*(volatile uint8_t*)0x4007F008)
#define RCM_SSRS1   (*(volatile uint8_t*)0x4007F009)

// PMC (Power Management Controller) registers
#define PMC_LVDSC1  (*(volatile uint8_t*)0x4007D000)
#define PMC_LVDSC2  (*(volatile uint8_t*)0x4007D001)
#define PMC_REGSC   (*(volatile uint8_t*)0x4007D002)

// SCB (System Control Block) registers
#define SCB_HFSR    (*(volatile uint32_t*)0xE000ED2C)
#define SCB_CFSR    (*(volatile uint32_t*)0xE000ED28)
#define SCB_BFAR    (*(volatile uint32_t*)0xE000ED38)
#define SCB_MMFAR   (*(volatile uint32_t*)0xE000ED34)

#define POWER_ON_RESET (RCM_SRS0 & 0x80)

bool Trace_LooksLikeNaturalCauses(void) {
    return (RCM_SRS0 & 0x80);
}

void Trace_PrintUhk60ReasonRegisters(device_id_t targetDeviceId, log_target_t targetInterface)
{
    LogTo(targetDeviceId, targetInterface, "=== K22P121M120SF7 Reset Cause Debug ===\n");

    // RCM registers
    LogTo(targetDeviceId, targetInterface, "RCM_SRS0:  0x%02X\n", RCM_SRS0);
    LogTo(targetDeviceId, targetInterface, "RCM_SRS1:  0x%02X\n", RCM_SRS1);
    LogTo(targetDeviceId, targetInterface, "RCM_SSRS0: 0x%02X\n", RCM_SSRS0);
    LogTo(targetDeviceId, targetInterface, "RCM_SSRS1: 0x%02X\n", RCM_SSRS1);

    // PMC registers
    LogTo(targetDeviceId, targetInterface, "PMC_LVDSC1: 0x%02X\n", PMC_LVDSC1);
    LogTo(targetDeviceId, targetInterface, "PMC_LVDSC2: 0x%02X\n", PMC_LVDSC2);
    LogTo(targetDeviceId, targetInterface, "PMC_REGSC:  0x%02X\n", PMC_REGSC);

    // SCB fault registers
    LogTo(targetDeviceId, targetInterface, "SCB_HFSR:  0x%08X\n", SCB_HFSR);
    LogTo(targetDeviceId, targetInterface, "SCB_CFSR:  0x%08X\n", SCB_CFSR);
    LogTo(targetDeviceId, targetInterface, "SCB_BFAR:  0x%08X\n", SCB_BFAR);
    LogTo(targetDeviceId, targetInterface, "SCB_MMFAR: 0x%08X\n", SCB_MMFAR);

    LogTo(targetDeviceId, targetInterface, "\n=== Reset Cause Analysis ===\n");

    // Quick analysis of reset causes
    if (RCM_SRS0 & 0x80) LogTo(targetDeviceId, targetInterface, "Power-On Reset detected\n");
    if (RCM_SRS0 & 0x40) LogTo(targetDeviceId, targetInterface, "External Pin Reset detected\n");
    if (RCM_SRS0 & 0x20) LogTo(targetDeviceId, targetInterface, "Watchdog Reset detected\n");
    if (RCM_SRS0 & 0x04) LogTo(targetDeviceId, targetInterface, "Low-Voltage Detect Reset detected\n");
    if (RCM_SRS0 & 0x02) LogTo(targetDeviceId, targetInterface, "Loss of Clock Reset detected\n");

    if (RCM_SRS1 & 0x20) LogTo(targetDeviceId, targetInterface, "Software Reset detected\n");

    // This is misleading;
    //if (RCM_SRS1 & 0x04) LogTo(targetDeviceId, targetInterface, "Core Lockup Reset detected\n");

    if (PMC_LVDSC1 & 0x80) LogTo(targetDeviceId, targetInterface, "Low-Voltage Detect Flag set\n");
    if (PMC_LVDSC2 & 0x80) LogTo(targetDeviceId, targetInterface, "Low-Voltage Warning Flag set\n");

    if (SCB_HFSR & 0x40000000) LogTo(targetDeviceId, targetInterface, "Forced HardFault detected\n");
}
#endif

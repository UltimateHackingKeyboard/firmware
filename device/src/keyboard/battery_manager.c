#include "battery_manager.h"
#include "config_manager.h"

    // TODO:
    // - adaptive detection of 100%
    // - logarithmic correction

typedef enum {
    ChargeRegion_Empty = 0,
    ChargeRegion_AlmostEmpty = 1,
    ChargeRegion_Powersaving = 2,
    ChargeRegion_PowersavingThreshold = 3,
    ChargeRegion_Moderate = 4,
    ChargeRegion_Storage = 5,
    ChargeRegion_High = 6,
} charge_region_t;

battery_manager_config_t BatteryManager_StandardUse = {
    .maxVoltage = 4000,
    .stopChargingVoltage = 4100,
    .startChargingVoltage = 4000,
    .turnOnBacklightVoltage = 3550,
    .turnOffBacklightVoltage = 3400,
    .minWakeupVoltage = 3100,
    .minVoltage = 3000,
};

battery_manager_config_t BatteryManager_LongLife = {
    .maxVoltage = 4000,
    .stopChargingVoltage =  3850,
    .startChargingVoltage = 3750,
    .turnOnBacklightVoltage = 3550,
    .turnOffBacklightVoltage = 3400,
    .minWakeupVoltage = 3100,
    .minVoltage = 3000,
};

static charge_region_t getCurrentChargeRegion(
        uint16_t voltage,
        battery_manager_config_t* config
) {
    if (voltage < config->minVoltage) {
        return ChargeRegion_Empty;
    } else if (voltage < config->minWakeupVoltage) {
        return ChargeRegion_AlmostEmpty;
    } else if (voltage < config->turnOffBacklightVoltage) {
        return ChargeRegion_Powersaving;
    } else if (voltage < config->turnOnBacklightVoltage) {
        return ChargeRegion_PowersavingThreshold;
    } else if (voltage < config->startChargingVoltage) {
        return ChargeRegion_Moderate;
    } else if (voltage < config->stopChargingVoltage) {
        return ChargeRegion_Storage;
    } else {
        return ChargeRegion_High;
    }
}

battery_manager_automaton_state_t BatteryManager_UpdateState(
        battery_manager_automaton_state_t currentState,
        battery_state_t* batteryState,
        battery_manager_config_t* config
) {
    charge_region_t currentRegion = getCurrentChargeRegion(batteryState->batteryVoltage, config);

    if (currentState == BatteryManagerAutomatonState_TurnOff && currentRegion < config->minWakeupVoltage && !batteryState->powered) {
        return BatteryManagerAutomatonState_TurnOff;
    }

    switch (currentRegion) {
        case ChargeRegion_Empty:
            if (!batteryState->powered) {
                return BatteryManagerAutomatonState_TurnOff;
            } else {
                return BatteryManagerAutomatonState_Charging;
            }
        case ChargeRegion_AlmostEmpty:
            if (currentState == BatteryManagerAutomatonState_TurnOff || currentState == BatteryManagerAutomatonState_Charging) {
                return currentState;
            } else {
                return BatteryManagerAutomatonState_Charging;
            }
        case ChargeRegion_Powersaving:
            if (!batteryState->powered) {
                return BatteryManagerAutomatonState_Powersaving;
            } else {
                return BatteryManagerAutomatonState_Charging;
            }
        case ChargeRegion_PowersavingThreshold:
            if (!batteryState->powered) {
                return BatteryManagerAutomatonState_Charging;
            } else if (currentState == BatteryManagerAutomatonState_Powersaving || currentState == BatteryManagerAutomatonState_Charging) {
                return currentState;
            } else {
                return BatteryManagerAutomatonState_Charging;
            }
        case ChargeRegion_Moderate:
            return BatteryManagerAutomatonState_Charging;
        case ChargeRegion_Storage:
            if (currentState == BatteryManagerAutomatonState_Charged || currentState == BatteryManagerAutomatonState_Charging) {
                return currentState;
            } else {
                return BatteryManagerAutomatonState_Charging;
            }
            return currentState;
        case ChargeRegion_High:
            return BatteryManagerAutomatonState_Charged;
        default:
            printk("Unknown charge region\n");
            return BatteryManagerAutomatonState_Charging;
    }
}

battery_manager_config_t* BatteryManager_GetCurrentBatteryConfig(void) {
    return Cfg.BatteryStationaryMode ? &BatteryManager_LongLife : &BatteryManager_StandardUse;
}

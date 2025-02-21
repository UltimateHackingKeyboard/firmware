#include "battery_manager.h"

    // TODO:
    // - adaptive detection of 100%
    // - logarithmic correction

typedef enum {
    ChargeRegion_Empty,
    ChargeRegion_AlmostEmpty,
    ChargeRegion_Low,
    ChargeRegion_Storage,
    ChargeRegion_High,
} charge_region_t;

battery_manager_config_t BatteryManager_StandardUse = {
    .maxVoltage = 4000,
    .stopChargingVoltage = 4100,
    .startChargingVoltage = 3900,
    .minWakeupVoltage = 3300,
    .minVoltage = 3200,
};

battery_manager_config_t BatteryManager_LongLife = {
    .maxVoltage = 4000,
    .stopChargingVoltage =  3850,
    .startChargingVoltage = 3750,
    .minWakeupVoltage = 3300,
    .minVoltage = 3200,
};

static charge_region_t getCurrentChargeRegion(
        uint16_t voltage,
        battery_manager_config_t* config
) {
    if (voltage < config->minVoltage) {
        return ChargeRegion_Empty;
    } else if (voltage < config->minWakeupVoltage) {
        return ChargeRegion_AlmostEmpty;
    } else if (voltage < config->startChargingVoltage) {
        return ChargeRegion_Low;
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
            return BatteryManagerAutomatonState_TurnOff;
        case ChargeRegion_AlmostEmpty:
            if (currentState == BatteryManagerAutomatonState_TurnOff || currentState == BatteryManagerAutomatonState_Charging) {
                return currentState;
            } else {
                return BatteryManagerAutomatonState_Charging;
            }
        case ChargeRegion_Low:
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

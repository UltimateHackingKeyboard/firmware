#ifndef __BATTERY_MANAGER_H__
#define __BATTERY_MANAGER_H__

// Includes:

    #include "charger.h"
    #include <zephyr/drivers/adc.h>
    #include <inttypes.h>
    #include <stdbool.h>

// Macros:

// Typedefs:

    typedef enum {
        BatteryManagerAutomatonState_TurnOff,
        BatteryManagerAutomatonState_Charging,
        BatteryManagerAutomatonState_Charged,
    } battery_manager_automaton_state_t;

    typedef struct {
        uint16_t maxVoltage;               // voltage reported as 100%
        uint16_t stopChargingVoltage;      // maximum voltage to charge to
                                           // optimum storage voltage is 3.8V according to Claude
        uint16_t startChargingVoltage;
        uint16_t minWakeupVoltage;         // once the keyboard enters shut down mode, it won't wake up until it is either externally powered, or the battery voltage reaches this level
        uint16_t minVoltage;               // voltage reported as 0%, we shut down at this voltage
    } ATTR_PACKED battery_manager_config_t;

// Variables:

    extern battery_manager_config_t BatteryManager_StandardUse;
    extern battery_manager_config_t BatteryManager_LongLife;

// Functions:

    battery_manager_automaton_state_t BatteryManager_UpdateState(battery_manager_automaton_state_t currentState, battery_state_t* batteryState, battery_manager_config_t* config);

#endif // __BATTERY_MANAGER_H__

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/util.h>
#include "charger.h"
#include "keyboard/charger.h"
#include "nrf52840.h"
#include "power_mode.h"
#include "shell.h"
#include "timer.h"
#include "event_scheduler.h"
#include "state_sync.h"
#include <nrfx_power.h>
#include "zephyr/kernel.h"
#include <zephyr/bluetooth/services/bas.h>
#include "battery_manager.h"
#include "config_manager.h"
#include <zephyr/pm/pm.h>
#include <nrfx_power.h>

/**
 * chargerStatDt == 1 => (actually) not charging (e.g., fully charged, or no power provided)
 * chargerStatDt == 0 => (actually) charging
 * chargerStatDt oscillating => battery not present
 * chargerEn == 1 => charger enabled
 * chargerEn == 0 => charger diasbled
 * */


const struct gpio_dt_spec chargerEnDt = GPIO_DT_SPEC_GET(DT_ALIAS(charger_en), gpios);
const struct gpio_dt_spec chargerStatDt = GPIO_DT_SPEC_GET(DT_ALIAS(charger_stat), gpios);
struct gpio_callback callbackStruct;
const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

#define CHARGER_UPDATE_PERIOD 60000
#define CHARGER_STAT_PERIOD 700
#define CHARGER_STABILIZATION_PERIOD 500

static battery_state_t batteryState;

static uint32_t lastStatZeroTime = 0;
static uint32_t lastStatOneTime = 0;

static uint16_t buf;
static struct adc_sequence sequence = {
    .buffer = &buf,
    .buffer_size = sizeof(buf),
};

bool RunningOnBattery = false;
bool RightRunningOnBattery = false;

bool Charger_ChargingEnabled = true;

static bool stabilizationPause = false;
static uint8_t statsToIgnore = 0;

static battery_manager_automaton_state_t currentChargingAutomatonState = BatteryManagerAutomatonState_Charging;

static battery_manager_config_t* getCurrentBatteryConfig() {
    return Cfg.BatteryStationaryMode ? &BatteryManager_LongLife : &BatteryManager_StandardUse;
}

static bool setBatteryPresent(bool present) {
    if (batteryState.batteryPresent != present) {
        batteryState.batteryPresent = present;
        if (!present) {
            batteryState.batteryCharging = false;
            batteryState.batteryVoltage = 0;
            batteryState.batteryPercentage = 0;
        }
        return true;
    }
    return false;
}

static bool setActuallyCharging(bool charging) {
    if (batteryState.batteryCharging != charging) {
        batteryState.batteryCharging = charging;
        return true;
    }
    return false;
}

static bool setPercentage(uint16_t voltage, uint8_t perc) {
    batteryState.batteryVoltage = voltage;

    if (batteryState.batteryPercentage != perc) {
        batteryState.batteryPercentage = perc;
        return true;
    }
    return false;
}

static bool updateBatteryPresent() {
    uint32_t statPeriod = MIN((uint32_t)(lastStatZeroTime - lastStatOneTime), (uint32_t)(lastStatOneTime-lastStatZeroTime));
    uint32_t lastStat = MAX(lastStatZeroTime, lastStatOneTime);
    bool batteryOscilates = statPeriod < CHARGER_STAT_PERIOD;
    bool changedRecently = (CurrentTime - lastStat) < CHARGER_STAT_PERIOD;
    bool batteryMissing = (changedRecently && batteryOscilates);
    bool batteryPresent = !batteryMissing;
    return setBatteryPresent(batteryPresent);
}

static bool updatePowered() {
    bool powered = nrfx_power_usbstatus_get() > NRFX_POWER_USB_STATE_DISCONNECTED;
    if (batteryState.powered != powered) {
        batteryState.powered = powered;
        return true;
    }
    return false;
}

static uint16_t getVoltage() {
    // Wait for the battery to stabilize
    adc_read(adc_channel.dev, &sequence);
    int32_t val_mv = (int32_t)buf;
    adc_raw_to_millivolts_dt(&adc_channel, &val_mv);
    return (uint16_t)(VOLTAGE_DIVIDER_MULTIPLIER*val_mv);
}

static void printState(battery_state_t* state) {
    printk("Battery is present: %i, charging: %i, charger enabled: %i, at %imV (%i%%)\n", state->batteryPresent, state->batteryCharging, Charger_ChargingEnabled, state->batteryVoltage, state->batteryPercentage);
}

void Charger_PrintState() {
    printState(&batteryState);
}

void Charger_EnableCharging(bool enabled) {
    if (Charger_ChargingEnabled != enabled) {
        Charger_ChargingEnabled = enabled;
        gpio_pin_set_dt(&chargerEnDt, Charger_ChargingEnabled && !stabilizationPause);
    }
}

void updateChargerEnabled(battery_state_t *batteryState, battery_manager_config_t* config) {
    battery_manager_automaton_state_t newState = BatteryManager_UpdateState(
            currentChargingAutomatonState,
            batteryState,
            config
    );

    if (newState != currentChargingAutomatonState) {
        currentChargingAutomatonState = newState;
        switch (newState) {
            case BatteryManagerAutomatonState_TurnOff:
                PowerMode_ActivateMode(PowerMode_AutoShutDown, false);
                break;
            case BatteryManagerAutomatonState_Charging:
                Charger_EnableCharging(true);
                break;
            case BatteryManagerAutomatonState_Charged:
                Charger_EnableCharging(false);
                break;
        }
    }
}

void Charger_UpdateBatteryState() {
    static uint32_t stabilizationPauseStartTime = 0;
    static bool stateChanged = false;
    static bool previousCharging = false;
    static uint16_t previousVoltage = 0;

    stateChanged |= updateBatteryPresent();
    stateChanged |= updatePowered();

    if (batteryState.batteryPresent) {
        if (stabilizationPause) {
            if (CurrentTime < stabilizationPauseStartTime + CHARGER_STABILIZATION_PERIOD) {
                // This is a spurious wakeup due to stat change, ignore
                EventScheduler_Reschedule(CurrentTime + CHARGER_STAT_PERIOD, EventSchedulerEvent_UpdateBattery, "spurious wakeup in stabilization pause");
                return;
            }

            // actually measure voltage
            uint16_t rawVoltage = getVoltage();

            uint16_t voltage = rawVoltage;
            if (previousVoltage != 0 && previousCharging && MAX(rawVoltage-previousVoltage, previousVoltage-rawVoltage) < 500) {
                voltage = rawVoltage - ((previousVoltage - rawVoltage) / 3);
            }

            if (batteryState.batteryVoltage != 0) {
                voltage = (voltage + batteryState.batteryVoltage*3) / 4;
            }

            // TODO: add more accurate computation
            battery_manager_config_t* currentBatteryConfig = getCurrentBatteryConfig();
            uint16_t minCharge = currentBatteryConfig->minVoltage;
            uint16_t maxCharge = currentBatteryConfig->maxVoltage;
            uint8_t perc = MIN(100, 1 + 99*(MAX(voltage, minCharge)-minCharge) / (maxCharge - minCharge));
            stateChanged |= setPercentage(voltage, perc);

            if (voltage == 0) {
                // the value is not valid, try again
                EventScheduler_Schedule(CurrentTime + CHARGER_STAT_PERIOD, EventSchedulerEvent_UpdateBattery, "charger - voltage == 0");
                return;
            } else {
                // run the state automaton that decides when to charge
                updateChargerEnabled(&batteryState, currentBatteryConfig);

                // if we have changed charger state, update whether actually charging
                stateChanged |= setActuallyCharging(batteryState.batteryCharging && Charger_ChargingEnabled);

                // we are done, schedule the next update
                stabilizationPauseStartTime = CurrentTime;
                EventScheduler_Schedule(CurrentTime + CHARGER_UPDATE_PERIOD, EventSchedulerEvent_UpdateBattery, "charger - minute period");
                statsToIgnore = 1;
                gpio_pin_set_dt(&chargerEnDt, Charger_ChargingEnabled);
                stabilizationPause = false;
                // continue processing
            }
        } else {
            // check whether the charger is actually charging
            bool actuallyCharging = !gpio_pin_get_dt(&chargerStatDt);
            stateChanged |= setActuallyCharging(actuallyCharging && Charger_ChargingEnabled);

            if (actuallyCharging) {
                previousVoltage = getVoltage();
                previousCharging = true;
            } else {
                previousVoltage = 0;
                previousCharging = false;
            }

            // turn of charger and let the battery voltage stabilize for measurement; Store any state changes until the measurement is done, just then apply.
            stabilizationPause = true;
            statsToIgnore = 1;
            gpio_pin_set_dt(&chargerEnDt, false);

            EventScheduler_Reschedule(CurrentTime + CHARGER_STABILIZATION_PERIOD, EventSchedulerEvent_UpdateBattery, "start stabilization pause");
            return;
        }
    } else {
        previousVoltage = 0;
    }

    if (stateChanged) {
        StateSync_UpdateProperty(StateSyncPropertyId_Battery, &batteryState);

#ifdef CONFIG_BT_BAS
        bt_bas_set_battery_level(batteryState.batteryPercentage);
#endif
        stateChanged = false;
    }
}

typedef enum {
    BatteryUpdateAutomatonState_MeasurePause,
    BatteryUpdateAutomatonState_Charging,
    BatteryUpdateAutomatonState_NotCharging,
} update_automaton_state_t;

static nrfx_power_usb_event_handler_t originalPowerHandler = NULL;

static void powerCallback(nrfx_power_usb_evt_t event) {
    originalPowerHandler(event);
    CurrentTime = k_uptime_get_32();
    EventScheduler_Schedule(CurrentTime + CHARGER_STAT_PERIOD, EventSchedulerEvent_UpdateBattery, "charger - power callback");
}

void chargerStatCallback(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins) {
    if (statsToIgnore > 0) {
        statsToIgnore--;
        return;
    }

    bool stat = gpio_pin_get_dt(&chargerStatDt);
    CurrentTime = k_uptime_get_32();
    if (stat) {
        lastStatOneTime = CurrentTime;
    } else {
        lastStatZeroTime = CurrentTime;
    }
    bool stateChanged = false;
    stateChanged |= updateBatteryPresent();
    stateChanged |= updatePowered();
    if (stateChanged) {
        StateSync_UpdateProperty(StateSyncPropertyId_Battery, &batteryState);
    }
    if (Shell.statLog) {
        printk("STAT changed to %i\n", stat ? 1 : 0);
    }
    EventScheduler_Reschedule(CurrentTime + CHARGER_STAT_PERIOD, EventSchedulerEvent_UpdateBattery, "charger - stat callback");
}

bool Charger_ShouldRemainInDepletedMode(bool checkVoltage) {
    updatePowered();
    if (checkVoltage) {
        uint16_t voltage = getVoltage();
        printk("Should remain in depleted mode because powered = %d && voltage = %d\n", batteryState.powered, voltage);
        return !batteryState.powered && voltage > 1000 && voltage < getCurrentBatteryConfig()->minWakeupVoltage;
    } else {
        printk("Should remain in depleted mode because powered = %d\n", batteryState.powered);
        return !batteryState.powered;
    }
}

bool Charger_ShouldEnterDepletedMode() {
    updatePowered();
    uint16_t voltage = getVoltage();
    return !batteryState.powered && voltage < getCurrentBatteryConfig()->minVoltage;
}

void InitCharger_Min(void) {
    adc_channel_setup_dt(&adc_channel);
    (void)adc_sequence_init_dt(&adc_channel, &sequence);
}

void InitCharger(void) {
    InitCharger_Min();

    gpio_pin_configure_dt(&chargerEnDt, GPIO_OUTPUT);
    Charger_EnableCharging(true);

    gpio_pin_configure_dt(&chargerStatDt, GPIO_INPUT);
    gpio_pin_interrupt_configure_dt(&chargerStatDt, GPIO_INT_EDGE_BOTH);
    gpio_init_callback(&callbackStruct, chargerStatCallback, BIT(chargerStatDt.pin));
    gpio_add_callback(chargerStatDt.port, &callbackStruct);

    const nrfx_power_usbevt_config_t config = {
        .handler = &powerCallback
    };

    originalPowerHandler = nrfx_power_usb_handler_get();
    nrfx_power_usbevt_init(&config);
    nrfx_power_usbevt_enable();

    EventScheduler_Reschedule(CurrentTime + CHARGER_STAT_PERIOD, EventSchedulerEvent_UpdateBattery, "charger - init");

    // TODO: Update battery level. See bas_notify()
}


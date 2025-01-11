#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/util.h>
#include "charger.h"
#include "keyboard/charger.h"
#include "shell.h"
#include "timer.h"
#include "attributes.h"
#include "event_scheduler.h"
#include "state_sync.h"
#include <nrfx_power.h>
#include "device.h"
#include <zephyr/bluetooth/services/bas.h>

const struct gpio_dt_spec chargerEnDt = GPIO_DT_SPEC_GET(DT_ALIAS(charger_en), gpios);
const struct gpio_dt_spec chargerStatDt = GPIO_DT_SPEC_GET(DT_ALIAS(charger_stat), gpios);
struct gpio_callback callbackStruct;
const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

#define CHARGER_UPDATE_PERIOD 60000
#define CHARGER_STAT_PERIOD 700

static battery_state_t batteryState;

static uint16_t minCharge = 3000;
static uint16_t maxCharge = 4000;

static uint32_t lastStatZeroTime = 0;
static uint32_t lastStatOneTime = 0;

static uint16_t buf;
static struct adc_sequence sequence = {
    .buffer = &buf,
    .buffer_size = sizeof(buf),
};

bool RunningOnBattery = false;
bool RightRunningOnBattery = false;

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

static bool setCharging(bool charging) {
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
    adc_read(adc_channel.dev, &sequence);
    int32_t val_mv = (int32_t)buf;
    adc_raw_to_millivolts_dt(&adc_channel, &val_mv);
    return (uint16_t)(VOLTAGE_DIVIDER_MULTIPLIER*val_mv);
}

void Charger_PrintState() {
    printk("Battery is present: %i, charging: %i, at %imV (%i%%)\n", batteryState.batteryPresent, batteryState.batteryCharging, batteryState.batteryVoltage, batteryState.batteryPercentage);
}

void Charger_UpdateBatteryState() {
    bool stateChanged = false;
    stateChanged |= updateBatteryPresent();
    stateChanged |= updatePowered();

    if (batteryState.batteryPresent) {
        bool charging = !gpio_pin_get_dt(&chargerStatDt);

        stateChanged |= setCharging(charging);

        uint16_t voltage = getVoltage();

        // TODO: add more accurate computation
        uint8_t perc = MIN(100, 100*(MAX(voltage, minCharge)-minCharge) / (maxCharge - minCharge));
        stateChanged |= setPercentage(voltage, perc);

        if (voltage == 0) {
            EventScheduler_Schedule(CurrentTime + CHARGER_STAT_PERIOD, EventSchedulerEvent_UpdateBattery, "charger - voltage == 0");
        } else {
            EventScheduler_Schedule(CurrentTime + CHARGER_UPDATE_PERIOD, EventSchedulerEvent_UpdateBattery, "charger - minute period");
        }
    }

    if (stateChanged) {
        StateSync_UpdateProperty(StateSyncPropertyId_Battery, &batteryState);

#ifdef CONFIG_BT_BAS
        bt_bas_set_battery_level(batteryState.batteryPercentage);
#endif
    }
}

static nrfx_power_usb_event_handler_t originalPowerHandler = NULL;

static void powerCallback(nrfx_power_usb_evt_t event) {
    originalPowerHandler(event);
    CurrentTime = k_uptime_get_32();
    EventScheduler_Schedule(CurrentTime + CHARGER_STAT_PERIOD, EventSchedulerEvent_UpdateBattery, "charger - power callback");
}

void chargerStatCallback(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins) {
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

// charging battery with CHARGER_EN yields STAT 0
// fully charged battery with CHARGER_EN yields STAT 1
// CHARGER_EN 0 yields STAT 1

void InitCharger(void) {
    gpio_pin_configure_dt(&chargerEnDt, GPIO_OUTPUT);
    gpio_pin_set_dt(&chargerEnDt, true);

    gpio_pin_configure_dt(&chargerStatDt, GPIO_INPUT);
    gpio_pin_interrupt_configure_dt(&chargerStatDt, GPIO_INT_EDGE_BOTH);
    gpio_init_callback(&callbackStruct, chargerStatCallback, BIT(chargerStatDt.pin));
    gpio_add_callback(chargerStatDt.port, &callbackStruct);

    adc_channel_setup_dt(&adc_channel);

    (void)adc_sequence_init_dt(&adc_channel, &sequence);

    const nrfx_power_usbevt_config_t config = {
        .handler = &powerCallback
    };

    originalPowerHandler = nrfx_power_usb_handler_get();
    nrfx_power_usbevt_init(&config);
    nrfx_power_usbevt_enable();

    EventScheduler_Reschedule(CurrentTime + CHARGER_STAT_PERIOD, EventSchedulerEvent_UpdateBattery, "charger - init");

    // TODO: Update battery level. See bas_notify()
}

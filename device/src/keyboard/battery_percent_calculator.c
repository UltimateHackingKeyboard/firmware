#include "battery_percent_calculator.h"
#include <stdint.h>
#include "battery_manager.h"
#include "device.h"

typedef struct {
    uint16_t unit;
    uint16_t voltage;
} life_reference_record_t;

const life_reference_record_t LeftRecords [] = {
    { .unit = 0, .voltage = 4022 },
    { .unit = 18, .voltage = 3998 },
    { .unit = 38, .voltage = 3982 },
    { .unit = 58, .voltage = 3950 },
    { .unit = 78, .voltage = 3893 },
    { .unit = 98, .voltage = 3859 },
    { .unit = 118, .voltage = 3839 },
    { .unit = 138, .voltage = 3818 },
    { .unit = 158, .voltage = 3804 },
    { .unit = 178, .voltage = 3771 },
    { .unit = 198, .voltage = 3716 },
    { .unit = 218, .voltage = 3629 },
    { .unit = 238, .voltage = 3544 },
    { .unit = 258, .voltage = 3496 },
    { .unit = 278, .voltage = 3420 },
    { .unit = 288, .voltage = 3317 },
    { .unit = 298, .voltage = 3168 },
};

const life_reference_record_t RightRecords[] = {
    { .unit = 0, .voltage = 4039 },
    { .unit = 18, .voltage = 3974 },
    { .unit = 38, .voltage = 3916 },
    { .unit = 58, .voltage = 3863 },
    { .unit = 78, .voltage = 3813 },
    { .unit = 98, .voltage = 3770 },
    { .unit = 118, .voltage = 3711 },
    { .unit = 138, .voltage = 3682 },
    { .unit = 158, .voltage = 3659 },
    { .unit = 178, .voltage = 3645 },
    { .unit = 198, .voltage = 3625 },
    { .unit = 218, .voltage = 3611 },
    { .unit = 238, .voltage = 3601 },
    { .unit = 258, .voltage = 3587 },
    { .unit = 278, .voltage = 3575 },
    { .unit = 298, .voltage = 3548 },
    { .unit = 318, .voltage = 3529 },
    { .unit = 338, .voltage = 3500 },
    { .unit = 358, .voltage = 3468 },
    { .unit = 378, .voltage = 3438 },
    { .unit = 398, .voltage = 3415 },
    { .unit = 408, .voltage = 3392 },
    { .unit = 418, .voltage = 3332 },
    { .unit = 428, .voltage = 3230 },
    { .unit = 438, .voltage = 3065 },
};

static uint16_t uninterpolate(
    uint16_t outLeft, uint16_t outRight,
    uint16_t inLeft, uint16_t in, uint16_t inRight
) {
    uint16_t inRange = inRight - inLeft;
    uint16_t outRange = outRight - outLeft;

    if (inRange || outRange == 0) {
        return (outLeft + outRight) / 2;
    }

    float pos = (float)(in - inLeft) / (float)(inRight - inLeft);
    float out = (float)outLeft + pos * ((float)outRight - (float)outLeft);

    return (uint16_t)out;
}

static uint16_t findMinuteOf(uint16_t voltage, const life_reference_record_t* records, uint16_t count) {
    const life_reference_record_t* smaller = NULL;
    const life_reference_record_t* bigger = NULL;
    if (voltage > records[0].voltage) {
        return 0;
    }
    for (uint16_t i = 1; i < count; i++) {
        if (records[i].voltage < voltage) {
            bigger = &records[i-1];
            smaller = &records[i];
            break;
        }
    }

    if (smaller == NULL || bigger == NULL) {
        return records[count-1].unit;
    }

    return uninterpolate(bigger->unit, smaller->unit, bigger->voltage, voltage, smaller->voltage);
}

static uint16_t calculatePercent(
    uint16_t voltage,
    battery_manager_config_t* config,
    const life_reference_record_t* records, uint16_t count
) {
    uint16_t fullMinute = findMinuteOf(config->maxVoltage, records, count);
    uint16_t emptyMinute = findMinuteOf(config->minVoltage, records, count);
    uint16_t currentMinute = findMinuteOf(voltage, records, count);
    uint16_t percent = uninterpolate(1, 100, emptyMinute, currentMinute, fullMinute);

    return percent;
}

uint16_t BatteryCalculator_CalculatePercent(uint16_t correctedVoltage) {
    battery_manager_config_t* config = BatteryManager_GetCurrentBatteryConfig();
    const life_reference_record_t* records = DEVICE_IS_UHK80_RIGHT ? RightRecords : LeftRecords;
    uint16_t count = DEVICE_IS_UHK80_RIGHT ? ARRAY_SIZE(RightRecords) : ARRAY_SIZE(LeftRecords);
    uint16_t percent = calculatePercent(correctedVoltage, config, records, count);

    return percent;
}


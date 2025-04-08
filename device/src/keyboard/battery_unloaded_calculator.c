#include "battery_unloaded_calculator.h"
#include "leds.h"
#include "device.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(Battery);

static bool testing = false;

// TODO: supply better data

const resistance_reference_t ReferenceResistanceRight = {
    .keyCount = 56,
    .brightnessSum = 255*3*56,
    .loadedVoltage = 3945,
    .unloadedVoltage = 4000,
    .loadedCurrent = 344,
    .unloadedCurrent = 30,
};

const resistance_reference_t ReferenceResistanceLeft = {
    .keyCount = 40,
    .brightnessSum = 255*3*40,
    .loadedVoltage = 3940,
    .unloadedVoltage = 4000,
    .loadedCurrent = 260,
    .unloadedCurrent = 30,
};

static double calculateLedResistance(const resistance_reference_t* ref, double referenceResistance, uint32_t brightnessSum) {
    double ledResistance = referenceResistance * (ref->brightnessSum+1) / (brightnessSum+1);
    return MIN(1000.0, ledResistance);
}

static double getInternalResistance(const resistance_reference_t* ref, double referenceResistance) {
    uint16_t voltageDiff = ref->unloadedVoltage - ref->loadedVoltage;
    uint16_t currentDiff = ref->loadedCurrent - ref->unloadedCurrent;
    ATTR_UNUSED double internalResistance = (double)voltageDiff / (double)currentDiff;
    // return 0.4;
    return internalResistance;
}

static double calculateUnloadedVoltageFromInternalResistance(double rawVoltage, double externalResistance, double internalResistance) {
    return rawVoltage + internalResistance*(rawVoltage / externalResistance);
}

static uint16_t calculateUnloadedVoltage(const resistance_reference_t* ref, uint16_t rawVoltage, uint32_t brightnessSum) {
    double baseResistance = 200;
    double referenceResistance = (double)ref->loadedVoltage / (double)ref->loadedCurrent;
    double ledResistance = calculateLedResistance(ref, referenceResistance, brightnessSum);
    double internalResistance = getInternalResistance(ref, referenceResistance);
    double externalResistance = (baseResistance*ledResistance)/(baseResistance + ledResistance);
    double unloadedVoltage = calculateUnloadedVoltageFromInternalResistance(rawVoltage, externalResistance, internalResistance);

    if (testing || true) {
        uint32_t iIntRes = (uint32_t)(internalResistance * 1000);
        uint32_t iLedRes = (uint32_t)(ledResistance * 1000);
        uint32_t iExtRes = (uint32_t)(externalResistance * 1000);
        uint32_t iUnloadedVoltage = (uint32_t)(unloadedVoltage);

        LOG_INF("  - Resistances %d %d %d, brightness %d, correcting %d -> %d\n",
            iIntRes,
            iLedRes,
            iExtRes,
            brightnessSum / ref->keyCount / 3,
            rawVoltage,
            iUnloadedVoltage);
    }

    return unloadedVoltage;
}

uint16_t BatteryCalculator_CalculateUnloadedVoltage(uint16_t rawVoltage) {
    const resistance_reference_t* ref = DEVICE_IS_UHK80_RIGHT ? &ReferenceResistanceRight : &ReferenceResistanceLeft;
    double brightnessSum = Leds_CalculateBrightnessSum();
    double unloadedVoltage = calculateUnloadedVoltage(ref, rawVoltage, brightnessSum);

    return unloadedVoltage;
}

void BatteryCalculator_RunTests(void) {
    const resistance_reference_t* ref;
    testing = true;

    LOG_INF("Running battery tests for Right:\n");
    ref = &ReferenceResistanceRight;
    calculateUnloadedVoltage(ref, 3500, ref->keyCount*3*255);
    calculateUnloadedVoltage(ref, 3500, ref->keyCount*3*128);
    calculateUnloadedVoltage(ref, 3500, ref->keyCount*3*64);
    calculateUnloadedVoltage(ref, 3500, ref->keyCount*3*8);
    LOG_INF("Running battery tests for Left:\n");
    ref = &ReferenceResistanceLeft;
    calculateUnloadedVoltage(ref, 3500, ref->keyCount*3*255);
    calculateUnloadedVoltage(ref, 3500, ref->keyCount*3*128);
    calculateUnloadedVoltage(ref, 3500, ref->keyCount*3*64);
    calculateUnloadedVoltage(ref, 3500, ref->keyCount*3*8);

    testing = false;
}

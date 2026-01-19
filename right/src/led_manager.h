#ifndef __LED_MANAGER_H__
#define __LED_MANAGER_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>
    #include "attributes.h"

// Macros:

    #define UHK60_LED_COUNT 6
    #define UHK60_SEGMENT_DISPLAY_CHARS 3
    #define UHK60_SEGMENT_COUNT_PER_CHAR 14
    #define UHK60_SEGMENT_DISPLAY_SIZE (UHK60_SEGMENT_DISPLAY_CHARS * UHK60_SEGMENT_COUNT_PER_CHAR)

    #define LED_VALUE_OFF 0
    #define LED_VALUE_ON 1
    #define LED_VALUE_UNCHANGED 2

// Typedefs:

    typedef struct {
        uint8_t mod : 1;
        uint8_t fn : 1;
        uint8_t mouse : 1;
        uint8_t capsLock : 1;
        uint8_t agent : 1;
        uint8_t adaptive : 1;
        uint8_t segmentDisplay : 1;
        uint8_t reserved : 1;
    } ATTR_PACKED led_override_uhk60_t;

    typedef struct {
        uint8_t leds[UHK60_LED_COUNT];                          // 6 bytes: mod, fn, mouse, capsLock, agent, adaptive
        uint8_t segments[UHK60_SEGMENT_DISPLAY_SIZE];           // 42 bytes: 3 chars * 14 segments
    } ATTR_PACKED uhk60_led_state_t;

// Variables:

    extern led_override_uhk60_t Uhk60LedOverride;
    extern uhk60_led_state_t Uhk60LedState;

    extern uint8_t DisplayBrightness;
    extern uint8_t KeyBacklightBrightness;

    extern bool KeyBacklightSleepModeActive;
    extern bool DisplaySleepModeActive;

    extern bool AlwaysOnMode;

// Functions:

    void LedManager_FullUpdate();
    void LedManager_RecalculateLedBrightness();
    void LedManager_UpdateAgentLed();
    void LedManager_UpdateSleepModes();


#endif

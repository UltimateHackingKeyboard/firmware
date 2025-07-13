#include <stdbool.h>
#include "merge_sensor.h"
#include "timer.h"
#include "event_scheduler.h"
#include "macro_events.h"

#ifdef __ZEPHYR__
#include <zephyr/drivers/gpio.h>
#include "state_sync.h"
#else
#include "fsl_gpio.h"
#include "fsl_port.h"

#define MERGE_SENSOR_GPIO        GPIOB
#define MERGE_SENSOR_PORT        PORTB
#define MERGE_SENSOR_CLOCK       kCLOCK_PortB
#define MERGE_SENSOR_PIN         3
#endif

#define MERGE_SENSOR_UPDATE_PERIOD 500

merge_sensor_state_t MergeSensor_HalvesAreMerged = MergeSensorState_Unknown;

#if (defined(__ZEPHYR__) && DEVICE_HAS_MERGE_SENSOR)
static const struct gpio_dt_spec mergeSenseDt = GPIO_DT_SPEC_GET(DT_ALIAS(merge_sense), gpios);
#endif

static void executeEffects(merge_sensor_state_t newState) {
#if (defined(__ZEPHYR__) && DEVICE_HAS_MERGE_SENSOR)
    StateSync_UpdateProperty(StateSyncPropertyId_MergeSensor, &MergeSensor_HalvesAreMerged);
#else
    MacroEvent_ProcessJoinSplitEvents(MergeSensor_HalvesAreMerged);
#endif
}

void MergeSensor_Update(void) {
    merge_sensor_state_t newState = MergeSensor_IsMerged();
    if (MergeSensor_HalvesAreMerged != newState && newState != MergeSensorState_Unknown) {
        MergeSensor_HalvesAreMerged = newState;
        executeEffects(newState);
    }
#if DEVICE_HAS_MERGE_SENSOR
    EventScheduler_Schedule(Timer_GetCurrentTime() + MERGE_SENSOR_UPDATE_PERIOD, EventSchedulerEvent_UpdateMergeSensor, "update merge sensor");
#endif
}

void MergeSensor_Init(void)
{
#if (defined(__ZEPHYR__) && DEVICE_HAS_MERGE_SENSOR)
    gpio_pin_configure_dt(&mergeSenseDt, GPIO_INPUT);
#endif

#if !defined(__ZEPHYR__)
    CLOCK_EnableClock(MERGE_SENSOR_CLOCK);
    PORT_SetPinConfig(MERGE_SENSOR_PORT, MERGE_SENSOR_PIN,
                      &(port_pin_config_t){.pullSelect=kPORT_PullUp, .mux=kPORT_MuxAsGpio});
#endif //__ZEPHYR__

#if DEVICE_HAS_MERGE_SENSOR
    EventScheduler_Schedule(Timer_GetCurrentTime() + MERGE_SENSOR_UPDATE_PERIOD, EventSchedulerEvent_UpdateMergeSensor, "init merge sensor");
#endif
}

merge_sensor_state_t MergeSensor_IsMerged(void)
{
#if (defined(__ZEPHYR__) && DEVICE_HAS_MERGE_SENSOR)
    return gpio_pin_get_dt(&mergeSenseDt) ? MergeSensorState_Joined : MergeSensorState_Split;
#elif (defined(__ZEPHYR__) && !DEVICE_HAS_MERGE_SENSOR)
    return MergeSensor_HalvesAreMerged;
#elif !defined(__ZEPHYR__)
    return !GPIO_PinRead(MERGE_SENSOR_GPIO, MERGE_SENSOR_PIN) ? MergeSensorState_Joined : MergeSensorState_Split;
#endif  //__ZEPHYR__
}

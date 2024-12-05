#include <stdbool.h>
#include "merge_sensor.h"
#include "timer.h"
#include "event_scheduler.h"

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
#define MERGE_SENSOR_IRQ         PORTB_IRQn
#define MERGE_SENSOR_IRQ_HANDLER PORTB_IRQHandler
#endif

#define MERGE_SENSOR_UPDATE_PERIOD 500

bool MergeSensor_HalvesAreMerged = false;

#if (defined(__ZEPHYR__) && DEVICE_HAS_MERGE_SENSOR)
static const struct gpio_dt_spec mergeSenseDt = GPIO_DT_SPEC_GET(DT_ALIAS(merge_sense), gpios);
#endif

void MergeSensor_Update(void) {
#if (defined(__ZEPHYR__) && DEVICE_HAS_MERGE_SENSOR)
    if (MergeSensor_HalvesAreMerged != MergeSensor_IsMerged()) {
        MergeSensor_HalvesAreMerged = !MergeSensor_HalvesAreMerged;
        StateSync_UpdateProperty(StateSyncPropertyId_MergeSensor, &MergeSensor_HalvesAreMerged);
    }
    EventScheduler_Schedule(CurrentTime + MERGE_SENSOR_UPDATE_PERIOD, EventSchedulerEvent_UpdateMergeSensor, "update merge sensor");
#endif
}

void MergeSensor_Init(void)
{
#if (defined(__ZEPHYR__) && DEVICE_HAS_MERGE_SENSOR)
    gpio_pin_configure_dt(&mergeSenseDt, GPIO_INPUT);
    EventScheduler_Schedule(CurrentTime + MERGE_SENSOR_UPDATE_PERIOD, EventSchedulerEvent_UpdateMergeSensor, "update merge sensor");
#elif (defined(__ZEPHYR__) && !DEVICE_HAS_MERGE_SENSOR)
    // Do nothing
#elif !defined(__ZEPHYR__)
    CLOCK_EnableClock(MERGE_SENSOR_CLOCK);
    PORT_SetPinConfig(MERGE_SENSOR_PORT, MERGE_SENSOR_PIN,
                      &(port_pin_config_t){.pullSelect=kPORT_PullUp, .mux=kPORT_MuxAsGpio});
#endif //__ZEPHYR__
}

bool MergeSensor_IsMerged(void)
{
#if (defined(__ZEPHYR__) && DEVICE_HAS_MERGE_SENSOR)
    return gpio_pin_get_dt(&mergeSenseDt);
#elif (defined(__ZEPHYR__) && !DEVICE_HAS_MERGE_SENSOR)
    return MergeSensor_HalvesAreMerged;
#elif !defined(__ZEPHYR__)
    return !GPIO_ReadPinInput(MERGE_SENSOR_GPIO, MERGE_SENSOR_PIN);
#endif  //__ZEPHYR__
}

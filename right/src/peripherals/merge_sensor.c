#include "merge_sensor.h"
#include "fsl_port.h"

void InitMergeSensor(void) {
    CLOCK_EnableClock(MERGE_SENSOR_CLOCK);
    PORT_SetPinConfig(MERGE_SENSOR_PORT, MERGE_SENSOR_PIN,
                      &(port_pin_config_t){.pullSelect=kPORT_PullUp, .mux=kPORT_MuxAsGpio});
}

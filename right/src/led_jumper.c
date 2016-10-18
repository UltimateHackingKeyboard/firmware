#include "led_jumper.h"
#include "fsl_port.h"

void InitLedJumper() {
    CLOCK_EnableClock(LED_JUMPER_CLOCK);
    PORT_SetPinConfig(LED_JUMPER_PORT, LED_JUMPER_PIN,
                      &(port_pin_config_t){.pullSelect=kPORT_PullUp, .mux=kPORT_MuxAsGpio});
}

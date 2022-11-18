#include <zephyr/types.h>

#define ADV_LED_BLINK_INTERVAL  1000

void bas_notify(void);
void button_changed(uint32_t button_state, uint32_t has_changed);
void bluetooth_init(void);
void bluetooth_set_adv_led(int blink_status);

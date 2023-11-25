#include <zephyr/types.h>

extern void bas_notify(void);
extern void bluetooth_init(void);
extern void num_comp_reply(uint8_t accept);
extern void key_report_send(uint8_t down);

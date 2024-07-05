#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include "device.h"

#if DEVICE_IS_UHK80_LEFT
    #define device_addr 0x18 // left module i2c address
#elif DEVICE_IS_UHK80_RIGHT || DEVICE_IS_UHK60V1_RIGHT || DEVICE_IS_UHK60V2_RIGHT
    #define device_addr 0x28 // right module i2c address
#endif

// Thread definitions

#define THREAD_STACK_SIZE 1000
#define THREAD_PRIORITY -1

static K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
static struct k_thread thread_data;

const struct device *i2c0_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

void i2cPoller() {
    uint8_t rx_buf[10] = {};
    struct i2c_msg msg;
    msg.buf = rx_buf;
    msg.len = 1;
    msg.flags = I2C_MSG_READ; // Unlike i2c_read(), don't use I2C_MSG_STOP

    while (true) {
        int ret = i2c_transfer(i2c0_dev, &msg, 1, device_addr);
        if (ret != 0) {
            printk("I2C read length error: %d\n", ret);
        }
        uint8_t msgLen = rx_buf[0];
        // printk("sync len: %d\n", msgLen);

        ret = i2c_read(i2c0_dev, rx_buf, msgLen+2, device_addr);
        if (ret != 0) {
            printk("write-read fail\n");
        }
        printk("buttons:%d x:%hd y:%hd\n", rx_buf[2], *(int16_t*)(&rx_buf[3]), *(int16_t*)(&rx_buf[5]));
        k_msleep(1000);
    }
}

void InitI2c(void) {
    uint8_t tx_buf[] = {/*length*/ 1, /* crc */ 0x10, 0x21, /* request key, pointer state */ 0x02};
    int ret;

    ret = i2c_write(i2c0_dev, tx_buf, sizeof(tx_buf), device_addr);
    if (ret != 0) {
        printk("i2c write error: %d\n", ret);
    }

    k_thread_create(
        &thread_data, stack_area,
        K_THREAD_STACK_SIZEOF(stack_area),
        i2cPoller,
        NULL, NULL, NULL,
        THREAD_PRIORITY, 0, K_NO_WAIT
    );

    k_thread_name_set(&thread_data, "i2c_poller");
}

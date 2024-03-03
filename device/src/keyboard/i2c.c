#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include "device.h"

void InitI2c(void) {
    // Init I2C
#if DEVICE_IS_UHK80_LEFT
    #define device_addr 0x18 // left module i2c address
#elif DEVICE_IS_UHK80_RIGHT || DEVICE_IS_UHK60V1_RIGHT || DEVICE_IS_UHK60V2_RIGHT
    #define device_addr 0x28 // right module i2c address
#endif

    uint8_t tx_buf[] = {0x00,0x00};
    uint8_t rx_buf[10] = {0};

    int ret;
    static const struct device *i2c0_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
    k_msleep(50);
    if (!device_is_ready(i2c0_dev)) {
        printk("I2C bus %s is not ready!\n",i2c0_dev->name);
    }

    ret = i2c_write_read(i2c0_dev, device_addr, tx_buf, 2, rx_buf, 7);
    if (ret != 0) {
        printk("write-read fail\n");
    }
    printk("sync: %.7s\n", rx_buf);
}

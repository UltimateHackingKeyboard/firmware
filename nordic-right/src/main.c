void main(void)
{


    uint32_t counter = 0;
    bool pixel = 1;
    while (true) {
        c = 0;
        for (uint8_t rowId=0; rowId<6; rowId++) {
            gpio_pin_set_dt(&rows[rowId], 1);
            for (uint8_t colId=0; colId<10; colId++) {
                if (gpio_pin_get_dt(&cols[colId])) {
                    c = 1;
                    printk("SW%c%c ", rowId+'1', colId+'1');
                }
            }
            gpio_pin_set_dt(&rows[rowId], 0);
        }

//      uart_poll_out(uart_dev, 'a');

        setA0(false);
        setOledCs(false);
        writeSpi(0xaf);
        setOledCs(true);

        setA0(false);
        setOledCs(false);
        writeSpi(0x81);
        writeSpi(0xff);
        setOledCs(true);

        setA0(true);
        setOledCs(false);
        writeSpi(pixel ? 0xff : 0x00);
        setOledCs(true);

        setLedsCs(false);
        writeSpi(LedPagePrefix | 2);
        writeSpi(0x00);
        writeSpi(0b00001001);
        setLedsCs(true);

        setLedsCs(false);
        writeSpi(LedPagePrefix | 2);
        writeSpi(0x01);
        writeSpi(0xff);
        setLedsCs(true);

        setLedsCs(false);
        writeSpi(LedPagePrefix | 0);
        writeSpi(0x00);
        for (int i=0; i<255; i++) {
            writeSpi(c?0xff:0);
        }
        setLedsCs(true);

        setLedsCs(false);
        writeSpi(LedPagePrefix | 1);
        writeSpi(0x00);
        for (int i=0; i<255; i++) {
            writeSpi(c?0xff:0);
        }
        setLedsCs(true);

//        k_msleep(1);

        if (counter++ > 19) {
            pixel = !pixel;
            counter = 0;
        }
    }
}

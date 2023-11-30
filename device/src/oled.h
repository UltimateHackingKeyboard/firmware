#ifndef __OLED_H__
#define __OLED_H__

// Variables:

    extern const struct gpio_dt_spec oledEn;
    extern const struct gpio_dt_spec oledResetDt;
    extern const struct gpio_dt_spec oledCsDt;
    extern const struct gpio_dt_spec oledA0Dt;

// Functions:

    extern void InitOled(void);

#endif // __OLED_H__

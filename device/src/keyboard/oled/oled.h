#ifndef __OLED_H__
#define __OLED_H__

// Includes:

    #include "widgets/widget.h"

// Variables:

    extern const struct gpio_dt_spec oledEn;
    extern const struct gpio_dt_spec oledResetDt;
    extern const struct gpio_dt_spec oledCsDt;
    extern const struct gpio_dt_spec oledA0Dt;

// Functions:

    void InitOled(void);
    void Oled_ActivateScreen(widget_t* screen, bool forceRedraw);
    void Oled_RequestRedraw(void);

#endif // __OLED_H__

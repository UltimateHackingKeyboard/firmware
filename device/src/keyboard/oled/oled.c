#include "device.h"
#include "framebuffer.h"
#include "oled.h"
#include "oled_buffer.h"
#include "screens/test_screen.h"
#include "widgets/widget.h"

#ifdef DEVICE_HAS_OLED

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "keyboard/oled/oled.h"
#include "keyboard/spi.h"

// Thread definitions

#define THREAD_STACK_SIZE 1000
#define THREAD_PRIORITY 5

static K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
static struct k_thread thread_data;

// OLED GPIOs

const struct gpio_dt_spec oledEn = GPIO_DT_SPEC_GET(DT_ALIAS(oled_en), gpios);
const struct gpio_dt_spec oledResetDt = GPIO_DT_SPEC_GET(DT_ALIAS(oled_reset), gpios);
const struct gpio_dt_spec oledCsDt = GPIO_DT_SPEC_GET(DT_ALIAS(oled_cs), gpios);
const struct gpio_dt_spec oledA0Dt = GPIO_DT_SPEC_GET(DT_ALIAS(oled_a0), gpios);

typedef enum {
    OledCommand_SetDisplayOn = 0xaf,
    OledCommand_SetContrast = 0x81,
    OledCommand_SetRowAddress = 0xb0,
    OledCommand_SetColumnLow = 0x00,
    OledCommand_SetColumnHigh = 0x10,
    OledCommand_SetScanDirectionDown = 0xc8,
} oled_commands_t;


static void setOledCs(bool state)
{
    gpio_pin_set_dt(&oledCsDt, state);
}

static void setA0(bool state)
{
    gpio_pin_set_dt(&oledA0Dt, state);
}

static void oledCommand1(bool A0, uint8_t b1) {
    setA0(A0);
    setOledCs(true);
    writeSpi(b1);
    setOledCs(false);
}

static void oledCommand2(bool A0, uint8_t b1, uint8_t b2) {
    setA0(A0);
    setOledCs(true);
    writeSpi(b1);
    writeSpi(b2);
    setOledCs(false);
}

static void setPositionTo(uint16_t x, uint16_t y, uint16_t lastWrittenPixelX, uint16_t lastWrittenPixelY)
{
    uint8_t columnAddress = (DISPLAY_WIDTH-1-x)/2;
    uint8_t rowAddress = y;

    if (lastWrittenPixelX != x+2 || lastWrittenPixelY != y) {
        setA0(0);
        if (lastWrittenPixelY != y) {
            writeSpi(OledCommand_SetRowAddress);
            writeSpi(rowAddress);
        }
        if (lastWrittenPixelX != x+2) {
            writeSpi(OledCommand_SetColumnLow | (columnAddress & 0x0f));
            writeSpi(OledCommand_SetColumnHigh | (columnAddress >> 4));
        }
        setA0(1);
    }
}

static bool oledNeedsRedraw = false;
static k_tid_t oledThreadId = 0;

static widget_t* currentScreen = NULL;
static uint16_t currentXShift = 0;
static uint16_t currentYShift = 0;

// Temporary code. If you see it in github, kick me hard.
static void forceRedraw() {
    for (uint16_t x = 0; x < 256; x++) {
        for (uint16_t y = 0; y < 64; y++) {
            OledBuffer->buffer[y*256+x] |= 1;
        }
    }
}

static void performScreenShift() {
    static uint16_t shiftCounter = 0;
    static bool initialized = false;

    if (!initialized) {
        shiftCounter = k_cycle_get_32() % (DISPLAY_SHIFTING_MARGIN*DISPLAY_SHIFTING_MARGIN);
        initialized = true;
    }

    shiftCounter++;

    if (shiftCounter > DISPLAY_SHIFTING_MARGIN*DISPLAY_SHIFTING_MARGIN) {
        shiftCounter = 0;
    }

    currentXShift = shiftCounter / (DISPLAY_SHIFTING_MARGIN);
    currentYShift = shiftCounter % (DISPLAY_SHIFTING_MARGIN);

    Framebuffer_Clear(NULL, OledBuffer);
    currentScreen->layOut(currentScreen, currentXShift, currentYShift, OledBuffer->width - DISPLAY_SHIFTING_MARGIN, OledBuffer->height - DISPLAY_SHIFTING_MARGIN);
}

static bool updateScreenShift() {
    static uint16_t counter1 = 0;

    if (counter1++ < 100) {
        return false;
    }

    counter1 = 0;

    performScreenShift();

    return true;
}

static void diffUpdate()
{
    k_mutex_lock(&SpiMutex, K_FOREVER);
    oledNeedsRedraw = false;

    setA0(true);
    setOledCs(true);

    for (uint16_t y = 0; y < OledBuffer->height; y++) {
        for (uint16_t x = OledBuffer->width-2; x < OledBuffer->width; x -= 2) {
            if (oledNeedsRedraw) {
                setOledCs(false);
                k_mutex_unlock(&SpiMutex);
                return;
            }

            static uint16_t lastWrittenPixelX = 0;
            static uint16_t lastWrittenPixelY = 0;

            uint8_t firstPixel = Framebuffer_GetPixel(OledBuffer, x, y);
            uint8_t secondPixel = Framebuffer_GetPixel(OledBuffer, x+1, y);

            if (firstPixel & Framebuffer_PixelIsDirty || secondPixel & Framebuffer_PixelIsDirty) {
                uint8_t upper = firstPixel >> 4;
                uint8_t lower = secondPixel & 0xf0;

                setPositionTo(x, y, lastWrittenPixelX, lastWrittenPixelY);

                writeSpi(upper | lower); //write pixel data
                lastWrittenPixelX = x;
                lastWrittenPixelY = y;
            }
        }
    }
    setOledCs(false);

    k_mutex_unlock(&SpiMutex);
}

void oledUpdater() {
    k_mutex_lock(&SpiMutex, K_FOREVER);
    oledCommand1(0, OledCommand_SetDisplayOn);
    oledCommand2(0, OledCommand_SetContrast, 0xff);
    oledCommand1(0, OledCommand_SetScanDirectionDown);
    k_mutex_unlock(&SpiMutex);

    forceRedraw();
    performScreenShift();
    currentScreen->draw(currentScreen, OledBuffer);

    while (true) {
        updateScreenShift();
        diffUpdate();

        if (!oledNeedsRedraw) {
            k_sleep(K_FOREVER);
        }

        currentScreen->draw(currentScreen, OledBuffer);
    }
}

void Oled_RequestRedraw() {
    oledNeedsRedraw = true;
    k_wakeup(oledThreadId);
}

void InitOled(void) {
    gpio_pin_configure_dt(&oledEn, GPIO_OUTPUT);
    gpio_pin_set_dt(&oledEn, true);

    gpio_pin_configure_dt(&oledResetDt, GPIO_OUTPUT);
    gpio_pin_set_dt(&oledResetDt, false);
    k_msleep(1);
    gpio_pin_set_dt(&oledResetDt, true);

    gpio_pin_configure_dt(&oledCsDt, GPIO_OUTPUT);
    gpio_pin_configure_dt(&oledA0Dt, GPIO_OUTPUT);

    OledBuffer_Init();
    TestScreen_Init(OledBuffer);
    currentScreen = TestScreen;

    oledThreadId = k_thread_create(
            &thread_data, stack_area,
            K_THREAD_STACK_SIZEOF(stack_area),
            oledUpdater,
            NULL, NULL, NULL,
            THREAD_PRIORITY, 0, K_NO_WAIT
            );
    k_thread_name_set(&thread_data, "oled_updater");
}

#endif // DEVICE_HAS_OLED

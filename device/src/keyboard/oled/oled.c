#include "device.h"
#include "framebuffer.h"
#include "oled.h"
#include "oled_display.h"
#include "oled_buffer.h"
#include "screens/screen_manager.h"
#include "event_scheduler.h"
#include "timer.h"
#include "led_manager.h"
#include "keyboard/oled/screens/screens.h"
#include "state_sync.h"

#if DEVICE_HAS_OLED

uint8_t OledOverrideMode = 0;

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "keyboard/oled/oled.h"
#include "keyboard/spi.h"

// Thread definitions

#define THREAD_STACK_SIZE 1000
#define THREAD_PRIORITY 5
#define OLED_FADE_TIME 2*255
#define OLED_FADE_STEP 1

static K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
static struct k_thread thread_data;

// OLED GPIOs

const struct gpio_dt_spec oledEn = GPIO_DT_SPEC_GET(DT_ALIAS(oled_en), gpios);
const struct gpio_dt_spec oledResetDt = GPIO_DT_SPEC_GET(DT_ALIAS(oled_reset), gpios);
const struct gpio_dt_spec oledCsDt = GPIO_DT_SPEC_GET(DT_ALIAS(oled_cs), gpios);
const struct gpio_dt_spec oledA0Dt = GPIO_DT_SPEC_GET(DT_ALIAS(oled_a0), gpios);

static uint8_t lastBrightness = 0xff;

static void setOledCs(bool state) {
    gpio_pin_set_dt(&oledCsDt, state);
}

static void setA0(bool state) {
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

static void setPositionTo(uint16_t x, uint16_t y, uint16_t lastWrittenPixelX, uint16_t lastWrittenPixelY) {
    uint8_t columnAddress = (DISPLAY_WIDTH-1-x)/2;

    if (lastWrittenPixelX != x+2 || lastWrittenPixelY != y) {
        setA0(0);

        uint8_t buf[4];
        uint8_t len;

        if (y != lastWrittenPixelY) {
            buf[0] = OledCommand_SetRowAddress;
            buf[1] = y;
            buf[2] = OledCommand_SetColumnLow | (columnAddress & 0x0f);
            buf[3] = OledCommand_SetColumnHigh | (columnAddress >> 4);
            len = 4;
        } else {
            buf[0] = OledCommand_SetColumnLow | (columnAddress & 0x0f);
            buf[1] = OledCommand_SetColumnHigh | (columnAddress >> 4);
            len = 2;
        }

        writeSpi2(buf, len);
        setA0(1);
    }
}

static bool oledNeedsRedraw = false;
static k_tid_t oledThreadId = 0;

static widget_t* currentScreen = NULL;
static uint16_t currentXShift = 0;
static uint16_t currentYShift = 0;
static bool wantScreenShift;

static uint8_t computeBrightness() {
    if (ActiveScreen == ScreenId_Debug && DisplayBrightness == 0) {
        return 255;
    } else if (StateSync_BatteryBacklightPowersavingMode) {
        return MIN(DisplayBrightness, 1);
    } else {
        return DisplayBrightness;
    }
}

static void forceRedraw() {
    for (uint16_t x = 0; x < DISPLAY_WIDTH; x+=2) {
        for (uint16_t y = 0; y < DISPLAY_HEIGHT; y++) {
            pixel_t* p = Framebuffer_GetPixel(OledBuffer, x, y);
            p->oldValue = p->value + 1;
        }
    }
}

void Oled_ActivateScreen(widget_t* screen, bool forceRedraw) {
    if (currentScreen != screen || forceRedraw) {
        currentScreen = screen;
        Framebuffer_Clear(NULL, OledBuffer);
        currentScreen->layOut(currentScreen, currentXShift, currentYShift, DISPLAY_WIDTH - DISPLAY_SHIFTING_MARGIN, DISPLAY_HEIGHT - DISPLAY_SHIFTING_MARGIN);
    }
}

void Oled_ForceRender() {
    Oled_ActivateScreen(currentScreen, true);
    Oled_RequestRedraw();
}

void Oled_UpdateBrightness() {
    Oled_RequestRedraw();
}

void Oled_ShiftScreen() {
    wantScreenShift = true;
    Oled_RequestRedraw();
}

static void performScreenShift() {
    static uint16_t shiftCounter = 0;
    static bool initialized = false;
    wantScreenShift = false;

    if (!initialized) {
        shiftCounter = k_cycle_get_32() % (DISPLAY_SHIFTING_MARGIN*DISPLAY_SHIFTING_MARGIN);
        initialized = true;
    }

    shiftCounter = (shiftCounter + 7) % (DISPLAY_SHIFTING_MARGIN*DISPLAY_SHIFTING_MARGIN);

    currentXShift = shiftCounter / (DISPLAY_SHIFTING_MARGIN);
    currentYShift = shiftCounter % (DISPLAY_SHIFTING_MARGIN);

    EventScheduler_Schedule(Timer_GetCurrentTime() + DISPLAY_SHIFTING_PERIOD, EventSchedulerEvent_ShiftScreen, "Oled - shift screen");
    Oled_ActivateScreen(currentScreen, true);
}

#define PIXEL(x, y) (OledBuffer->buffer[(y*DISPLAY_WIDTH+x)/2])

static bool shouldContinueWithoutPositionChange(uint16_t x, uint16_t y) {
    static uint8_t lastX = 255, lastY = 255;
    if (y == lastY && lastX < x) {
        return true;
    }
    uint16_t max = MIN(DISPLAY_MIN_GAP_TO_SKIP*2, x);
    for (uint8_t i = 2; i <= max; i += 2) {
        if (PIXEL(x-i, y).value != PIXEL(x-i, y).oldValue) {
            lastX = x-i;
            lastY = y;
            return true;
        }
    }
    return false;
}

static uint16_t roundToEven(uint16_t a) {
    return a & ~1;
}

static void setOledBrightness(uint8_t brightness) {
    if (brightness == 0) {
        oledCommand1(0, OledCommand_SetDisplayOff);
    } else {
        oledCommand1(0, OledCommand_SetDisplayOn);
        oledCommand2(0, OledCommand_SetContrast, brightness);
    }

    lastBrightness = brightness;
}

static void adjustBrightness() {
    uint8_t targetBrightness = computeBrightness();

    uint8_t nextBrightness = lastBrightness;


    if (nextBrightness != targetBrightness) {
        if (nextBrightness - OLED_FADE_STEP > targetBrightness) {
            nextBrightness -= OLED_FADE_STEP;
        } else if (nextBrightness + OLED_FADE_STEP < targetBrightness) {
            nextBrightness += OLED_FADE_STEP;
        } else {
            nextBrightness = targetBrightness;
        }
    }

    setOledBrightness(nextBrightness);
}

static void diffUpdate() {
    setA0(true);
    setOledCs(true);

    // uint32_t time = k_uptime_get_32();

    uint8_t buf[DISPLAY_WIDTH / 2];
    uint8_t buf_pos = 0;

    for (uint16_t y = 0; y < DISPLAY_HEIGHT; y++) {
        uint16_t min = roundToEven(OledBuffer->dirtyRanges[y].min);
        uint16_t max = roundToEven(OledBuffer->dirtyRanges[y].max);
        for (uint16_t x = max; min <= x && x < DISPLAY_WIDTH; x -= 2) {
            if (oledNeedsRedraw) {
                setOledCs(false);
                k_mutex_unlock(&SpiMutex);
                return;
            }

            static uint16_t lastWrittenPixelX = 0;
            static uint16_t lastWrittenPixelY = 0;

            pixel_t* pixel = &PIXEL(x, y);

            if (pixel->value != pixel->oldValue) {
                buf_pos = 0;

                setPositionTo(x, y, lastWrittenPixelX, lastWrittenPixelY);

                buf[buf_pos++] = pixel->value;
                pixel->oldValue = pixel->value;

                uint16_t changed = 0;

                while (shouldContinueWithoutPositionChange(x, y)) {
                    x -= 2;
                    buf[buf_pos++] = PIXEL(x, y).value;
                    PIXEL(x, y).oldValue = PIXEL(x, y).value;
                    changed++;
                }

                writeSpi2(buf, buf_pos);

                lastWrittenPixelX = x;
                lastWrittenPixelY = y;
            }
        }
        OledBuffer->dirtyRanges[y].min = 255;
        OledBuffer->dirtyRanges[y].max = 0;
    }
    setOledCs(false);
    // printk("update took %ims\n", k_uptime_get_32() - time);
}

void sleepDisplay() {
    while (computeBrightness() == 0 && lastBrightness == 0) {
        k_sleep(K_FOREVER);
    }
}

void oledUpdater() {
    k_mutex_lock(&SpiMutex, K_FOREVER);
    setOledBrightness(0);
    oledCommand1(0, OledCommand_SetScanDirectionDown);
    k_mutex_unlock(&SpiMutex);

    performScreenShift();
    currentScreen->draw(currentScreen, OledBuffer);
    forceRedraw();

    bool lastOledNeedsRedraw = true;

    while (true) {
        {
            k_mutex_lock(&SpiMutex, K_FOREVER);

            if (lastBrightness != computeBrightness()) {
                adjustBrightness();
            }

            if (lastOledNeedsRedraw) {
                lastOledNeedsRedraw = false;
                diffUpdate();
            }

            k_mutex_unlock(&SpiMutex);
        }

        if (!oledNeedsRedraw && lastBrightness == computeBrightness()) {
            k_sleep(K_FOREVER);
        } else {
            k_sleep(K_MSEC(OLED_FADE_TIME / (255/OLED_FADE_STEP)));
        }

        if (lastBrightness == 0) {
            sleepDisplay();
        }

        lastOledNeedsRedraw = oledNeedsRedraw;
        oledNeedsRedraw = false;

        if (wantScreenShift) {
            performScreenShift();
        }

        currentScreen->draw(currentScreen, OledBuffer);
    }
}

void Oled_RequestRedraw() {
    oledNeedsRedraw = true;
    if (oledThreadId) {
        k_wakeup(oledThreadId);
    }
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
    ScreenManager_Init();
    currentScreen = MainScreen;

    oledThreadId = k_thread_create(
            &thread_data, stack_area,
            K_THREAD_STACK_SIZEOF(stack_area),
            oledUpdater,
            NULL, NULL, NULL,
            THREAD_PRIORITY, 0, K_NO_WAIT
            );
    k_thread_name_set(&thread_data, "oled_updater");
}

#else // DEVICE_HAS_OLED

uint8_t OledOverrideMode = 0;

void Oled_ActivateScreen(widget_t* screen, bool forceRedraw){};
void Oled_RequestRedraw(){};
void Oled_ShiftScreen(){};

#endif // DEVICE_HAS_OLED

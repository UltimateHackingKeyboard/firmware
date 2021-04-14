#include "debug.h"

#ifdef WATCHES

#include "led_display.h"
#include "timer.h"
#include "key_states.h"

uint8_t CurrentWatch = 0;

static uint32_t lastWatch = 0;
static uint32_t watchInterval = 500;

static void ShowNumberExp(int32_t a)
{
    char b[3];
    int mag = 0;
    int num = a;
    if (num < 0) {
        LedDisplay_SetText(3, "NEG");
    } else {
        if (num < 1000) {
            b[0] = '0' + num / 100;
            b[1] = '0' + num % 100 / 10;
            b[2] = '0' + num % 10;
        } else {
            while (num >= 100) {
                mag++;
                num /= 10;
            }
            b[0] = '0' + num / 10;
            b[1] = '0' + num % 10;
            b[2] = mag == 0 ? '0' : ('A' - 1 + mag);
        }
        LedDisplay_SetText(3, b);
    }
}

void TriggerWatch(key_state_t *keyState)
{
    int16_t key = (keyState - &KeyStates[SlotId_LeftKeyboardHalf][0]);
    if (0 <= key && key <= 7) {
        // Set the LED value to RES until next update occurs.
        LedDisplay_SetText(3, "RES");
        CurrentWatch = key;
    }
}

void WatchTime(uint8_t n)
{
    static uint32_t lastUpdate = 0;
    if (CurrentTime - lastWatch > watchInterval) {
        ShowNumberExp(CurrentTime - lastUpdate);
        lastWatch = CurrentTime;
    }
    lastUpdate = CurrentTime;
}

void WatchValue(int v, uint8_t n)
{
    if (CurrentTime - lastWatch > watchInterval) {
        ShowNumberExp(v);
        lastWatch = CurrentTime;
    }
}

void ShowValue(int v, uint8_t n)
{
    ShowNumberExp(v);
}

void WatchString(char const *v, uint8_t n)
{
    if (CurrentTime - lastWatch > watchInterval) {
        LedDisplay_SetText(3, v);
        lastWatch = CurrentTime;
    }
}

void ShowString(char const *v, uint8_t n)
{
	LedDisplay_SetText(3, v);
}

#endif

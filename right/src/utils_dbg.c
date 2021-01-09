#include "utils_dbg.h"

#ifdef DEBUG

#include "led_display.h"
#include "timer.h"
#include "key_states.h"

uint8_t CurrentWatch = 0;

static uint32_t lastWatch = 0;
static uint32_t watchInterval = 500;

void TriggerWatch(key_state_t *keyState)
{
    int16_t key = (keyState - &KeyStates[SlotId_LeftKeyboardHalf][0]);
    if(0 <= key && key <= 7)
    {
        LedDisplay_SetText(3, "RES");
        CurrentWatch = key;
    }
}

void WatchTime(uint8_t n)
{
    static uint32_t lastUpdate = 0;
    if(CurrentTime - lastWatch > watchInterval) {
        ShowNumberMag(CurrentTime - lastUpdate);
        lastWatch = CurrentTime;
    }
    lastUpdate = CurrentTime;
}

void WatchValue(int v, uint8_t n)
{
    if(CurrentTime - lastWatch > watchInterval) {
        ShowNumberMag(v);
        lastWatch = CurrentTime;
    }
}


void ShowNumberMag(int a) {
    char b[3];
    int mag = 0;
    int num = a;
    while(num >= 100) {
        mag++;
        num /= 10;
    }
    b[0] = '0' + num/10;
    b[1] = '0' + num%10;
    b[2] = '0' + mag;
    LedDisplay_SetText(3,  b);
}

#endif

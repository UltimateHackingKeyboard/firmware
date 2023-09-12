#include "i2c_addresses.h"
#include "i2c.h"
#include "slave_scheduler.h"
#include "slave_drivers/uhk_module_driver.h"
#include "slave_protocol.h"
#include "peripherals/test_led.h"
#include "bool_array_converter.h"
#include "crc16.h"
#include "key_states.h"
#include "usb_interfaces/usb_interface_mouse.h"
#include "touchpad_driver.h"
#include "debug.h"
#include "timer.h"
#include "macros/core.h"

/*
|  Actually produced sequences:
| 1       single tap
| 2       tap and hold
|       1 number of fingers
|
| Tap:
|
| 0  0  0
| 0  0  1
| 1  0  0
|
|
| tap and hold:
|
| 0  0  0
| 0  0  1
| 2  0  1
|
| tap and hold (separate):
|
| 0  0  0
| 0  0  1
| 1  0  0
| 0  0  0
| 0  0  1
| 2  0  1
|
| two taps:
|
| 0  0  0
| 0  0  1
| 1  0  0
| 0  0  0
| 0  0  1
| 1  0  0
*/



typedef struct {
    struct {
        bool singleTap: 1;
        bool tapAndHold: 1;
        uint8_t unused: 6;
    } events0;
    struct {
        bool twoFingerTap : 1;
        bool scroll : 1;
        bool zoom : 1;
        uint8_t unused: 5;
    } events1;
} gesture_events_t;

static gesture_events_t gestureEvents;



uint8_t address = I2C_ADDRESS_RIGHT_IQS5XX_FIRMWARE;
touchpad_events_t TouchpadEvents;
const touchpad_events_t ZeroTouchpadEvents;
uint8_t phase = 0;
static uint8_t enableEventMode[] = {0x05, 0x8f, 0x07};

// Disable touchpad's default state transitions, in order to make it always
// listen on I2C.
//
// (By touchpad's original design, master is supposed to communicate to the touchpad
// only when the touchpad indicates that new data is available via its RDY pin...
// and NACKs otherwise)
static uint8_t enableManualMode[] = {0x05, 0x8e, 0xec};

// The touchpad wil *also* NACK if we ask it sooner than after the configured
// report rate, so we set it to 1ms so that it is always prepared.
static uint8_t setReportRate[] = {0x05, 0x7b, 0x01};

static uint8_t getGestureEvents0[] = {0x00, 0x0d};
static uint8_t getRelativePixelsXCommand[] = {0x00, 0x12};
static uint8_t closeCommunicationWindow[] = {0xee, 0xee, 0xee};
static uint8_t getNoFingers[] = {0x00, 0x11};
static uint8_t buffer[5];
uint8_t noFingers;
int16_t deltaX;
int16_t deltaY;

void TouchpadDriver_Init(uint8_t uhkModuleDriverId)
{
    phase = 0;
}

slave_result_t TouchpadDriver_Update(uint8_t uhkModuleDriverId)
{
    slave_result_t res = { .status = kStatus_Uhk_IdleSlave, .hold = true };

    switch (phase) {
        case 0: {
            res.status = I2cAsyncWrite(address, enableEventMode, sizeof(enableEventMode));
            phase = 1;
            break;
        }
        case 1: {
            res.status = I2cAsyncWrite(address, enableManualMode, sizeof(enableManualMode));
            ModuleConnectionStates[UhkModuleDriverId_RightModule].moduleId = ModuleId_TouchpadRight;
            phase = 2;
            break;
        }
        case 2: {
            res.status = I2cAsyncWrite(address, setReportRate, sizeof(setReportRate));
            phase = 3;
            break;
        }
        case 3: {
            res.status = I2cAsyncWrite(address, getGestureEvents0, sizeof(getGestureEvents0));
            phase = 4;
            break;
        }
        case 4: {
            res.status = I2cAsyncRead(address, (uint8_t*)&gestureEvents, sizeof(gesture_events_t));
            phase = 5;
            break;
        }
        case 5: {
            res.status = I2cAsyncWrite(address, getNoFingers, sizeof(getNoFingers));
            phase = 6;
            break;
        }
        case 6: {
            res.status = I2cAsyncRead(address, &noFingers, 1);
            phase = 7;
            break;
        }
        case 7: {
            res.status = I2cAsyncWrite(address, getRelativePixelsXCommand, sizeof(getRelativePixelsXCommand));
            phase = 8;
            break;
        }
        case 8: {
            res.status = I2cAsyncRead(address, buffer, 5);
            phase = 9;
            break;
        }
        case 9: {
            deltaY = (int16_t)(buffer[1] | buffer[0]<<8);
            deltaX = (int16_t)(buffer[3] | buffer[2]<<8);

            ModuleConnectionStates[UhkModuleDriverId_RightModule].lastTimeConnected = CurrentTime;

            TouchpadEvents.singleTap = gestureEvents.events0.singleTap;
            TouchpadEvents.twoFingerTap = gestureEvents.events1.twoFingerTap;
            TouchpadEvents.tapAndHold = gestureEvents.events0.tapAndHold;
            TouchpadEvents.noFingers = noFingers;

            if (gestureEvents.events1.scroll) {
                TouchpadEvents.wheelX -= deltaX;
                TouchpadEvents.wheelY += deltaY;
            } else if (gestureEvents.events1.zoom) {
                TouchpadEvents.zoomLevel -= deltaY;
            } else {
                TouchpadEvents.x -= deltaX;
                TouchpadEvents.y += deltaY;
            }

            res.status = I2cAsyncWrite(address, closeCommunicationWindow, sizeof(closeCommunicationWindow));
            res.hold = false;
            phase = 3;
            break;
        }
    }

    return res;
}

void TouchpadDriver_Disconnect(uint8_t uhkModuleDriverId)
{
    TouchpadEvents.x = 0;
    TouchpadEvents.y = 0;
    phase = 0;
}

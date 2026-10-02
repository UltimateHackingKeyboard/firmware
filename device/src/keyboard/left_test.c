#include "device.h"

// Left half only; the right would just carry it as dead code, and its flash is nearly full.
#if DEVICE_IS_UHK80_LEFT

#include <string.h>
#include "left_test.h"
#include "messenger.h"
#include "link_protocol.h"
#include "connections.h"
#include "module.h"
#include "slot.h"
#include "timer.h"
#include "logger.h"
#include "bool_array_converter.h"
#include "test_suite/tests/tests.h"
#include "test_suite/test_input_machine.h"

volatile bool LeftTest_Active = false;

static volatile uint8_t requestedTest = 0;
static uint8_t activeTest = 0;

static const test_action_t *script;
static uint16_t actionIndex;
static uint32_t delayStartedAt;
static bool inDelay;

// Which left-half key positions are currently held, indexed by universal key id - the same
// numbering scanAllKeys feeds to BoolBitToBytes.
static bool held[MAX_KEY_COUNT_PER_MODULE];

static uint32_t sendErrors;

void LeftTest_Start(uint8_t testId) {
    requestedTest = testId;
    LeftTest_Active = testId != 0;
}

static void restart(uint8_t testId) {
    activeTest = testId;
    script = LinkTest_GetActions(testId);
    actionIndex = 0;
    inDelay = false;
    memset(held, 0, sizeof(held));

    if (script == NULL) {
        LogU("LeftTest: no script for test %d\n", testId);
        activeTest = 0;
        requestedTest = 0;
        LeftTest_Active = false;
    } else {
        LogU("LeftTest: running test %d\n", testId);
    }
}

static void finish(void) {
    LogU("LeftTest: finished test %d\n", activeTest);
    memset(held, 0, sizeof(held));
    script = NULL;
    activeTest = 0;
    requestedTest = 0;
    LeftTest_Active = false;
}

// Runs the same script the right half runs, but honours only what this half is responsible
// for: its own key positions, and the delays that space them. Everything else - the right
// half's keys, the config actions, the expectations - is stepped over without consuming a
// tick, so both halves stay on the same timeline.
static void advance(void) {
    while (script != NULL) {
        const test_action_t *action = &script[actionIndex];

        if (action->type == TestAction_End) {
            finish();
            return;
        }

        if (action->type == TestAction_Delay) {
            if (!inDelay) {
                inDelay = true;
                delayStartedAt = Timer_GetCurrentTime();
                return;
            }
            if (Timer_GetElapsedTime(&delayStartedAt) < action->delayMs) {
                return;
            }
            inDelay = false;
            actionIndex++;
            continue;
        }

        bool isPress = action->type == TestAction_Press;
        bool isRelease = action->type == TestAction_Release;

        if (isPress || isRelease) {
            uint8_t slotId, keyId;
            bool resolved = TestInput_ParseKeyId(action->keyId, &slotId, &keyId);
            bool ours = resolved && slotId == SlotId_LeftKeyboardHalf
                && keyId < MAX_KEY_COUNT_PER_MODULE;

            if (ours) {
                held[keyId] = isPress;
            }
        }

        actionIndex++;
    }
}

void LeftTest_Tick(void) {
    if (requestedTest != activeTest) {
        restart(requestedTest);
    }

    advance();

    uint8_t compressedLength = MAX_KEY_COUNT_PER_MODULE/8+1;
    uint8_t compressedBuffer[compressedLength];
    memset(compressedBuffer, 0, compressedLength);

    for (uint8_t keyId = 0; keyId < MAX_KEY_COUNT_PER_MODULE; keyId++) {
        if (held[keyId]) {
            BoolBitToBytes(true, keyId, compressedBuffer);
        }
    }

    // Sent every tick rather than on change only: the point of the harness is to keep the
    // bridge busy, and a resend of an unchanged state is exactly what the right half has to
    // tolerate anyway.
    //
    // Pinned to the UART connection rather than left to determineChannel: with BLE to the
    // right also up, the default route can carry this over NUS instead, and then the harness
    // silently stresses the wrong link.
    int err = Messenger_Send2Via(DeviceId_Uhk80_Right, ConnectionId_UartRight,
            MessageId_SyncableProperty, SyncablePropertyId_LeftHalfKeyStates,
            compressedBuffer, compressedLength);

    if (err != 0 && sendErrors++ % 32 == 0) {
        LogU("LeftTest: uart send failed (%d), %d so far\n", err, sendErrors);
    }
}

#endif // DEVICE_IS_UHK80_LEFT

#include "device.h"

// Left half only; the right would just carry it as dead code, and its flash is nearly full.
#if DEVICE_IS_UHK80_LEFT

#include <string.h>
#include "left_test.h"
#include "device.h"
#include "messenger.h"
#include "link_protocol.h"
#include "connections.h"
#include "module.h"
#include "timer.h"
#include "logger.h"
#include "bool_array_converter.h"

// Key positions pressed by LeftTestId_TypePhrase, in order. Universal key ids, the same
// numbering scanAllKeys feeds to BoolBitToBytes.
static const uint8_t phraseKeys[] = { 0, 1, 2, 3, 4, 5, 6, 7 };

// Long enough to clear debouncing on the right half (Cfg.DebounceTimePress defaults well
// below this) while still being brisk enough to keep the link busy.
#define HOLD_MS 30
#define GAP_MS 30

volatile bool LeftTest_Active = false;

static volatile uint8_t requestedTest = LeftTestId_None;
static uint8_t activeTest = LeftTestId_None;

static uint32_t sendErrors;
static uint8_t keyIndex;
static bool holding;
static uint32_t phaseStartedAt;

void LeftTest_Start(uint8_t testId) {
    requestedTest = testId;
    LeftTest_Active = testId != LeftTestId_None;
}

static void beginPhase(bool hold) {
    holding = hold;
    phaseStartedAt = Timer_GetCurrentTime();
}

// Advances the press/release machine. Returns the key position to report as held, or
// MAX_KEY_COUNT_PER_MODULE for none.
static uint8_t advance(void) {
    bool exhausted = keyIndex >= sizeof(phraseKeys);

    if (exhausted) {
        LogU("LeftTest: finished test %d\n", activeTest);
        activeTest = LeftTestId_None;
        requestedTest = LeftTestId_None;
        LeftTest_Active = false;
        return MAX_KEY_COUNT_PER_MODULE;
    }

    uint32_t elapsed = Timer_GetElapsedTime(&phaseStartedAt);
    bool phaseOver = elapsed >= (holding ? HOLD_MS : GAP_MS);

    if (phaseOver) {
        if (holding) {
            keyIndex++;
            beginPhase(false);
        } else {
            beginPhase(true);
        }
    }

    return holding ? phraseKeys[keyIndex] : MAX_KEY_COUNT_PER_MODULE;
}

void LeftTest_Tick(void) {
    if (requestedTest != activeTest) {
        activeTest = requestedTest;
        keyIndex = 0;
        beginPhase(false);
        LogU("LeftTest: starting test %d\n", activeTest);
    }

    uint8_t heldKey = advance();

    uint8_t compressedLength = MAX_KEY_COUNT_PER_MODULE/8+1;
    uint8_t compressedBuffer[compressedLength];
    memset(compressedBuffer, 0, compressedLength);

    if (heldKey < MAX_KEY_COUNT_PER_MODULE) {
        BoolBitToBytes(true, heldKey, compressedBuffer);
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

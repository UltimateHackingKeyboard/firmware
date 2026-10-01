#ifndef __LEFT_TEST_H__
#define __LEFT_TEST_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>

// Typedefs:

    typedef enum {
        LeftTestId_None = 0,
        // Presses a run of left-half key positions one after another. What that spells is up
        // to the key actions the right half's test installs, so this carries no keymap
        // assumptions of its own.
        LeftTestId_TypePhrase = 1,
    } left_test_id_t;

// Variables:

    // True while a test is producing keys. The key scanner runs LeftTest_Tick instead of the
    // matrix scan while it is set.
    extern volatile bool LeftTest_Active;

// Functions:

    // Only flips state, so it is safe to call from the bridge's receive path (ISR context).
    void LeftTest_Start(uint8_t testId);

    // Runs on the key scanner thread, in place of scanAllKeys.
    void LeftTest_Tick(void);

#endif // __LEFT_TEST_H__

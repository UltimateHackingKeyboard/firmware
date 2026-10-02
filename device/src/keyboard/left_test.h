#ifndef __LEFT_TEST_H__
#define __LEFT_TEST_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>

// Variables:

    // True while a link test is producing keys. The key scanner runs LeftTest_Tick instead
    // of the matrix scan while it is set.
    extern volatile bool LeftTest_Active;

// Functions:

    // Starts the link test with the given id, or stops the running one when given 0. Ids are
    // the ones LinkTest_GetActions resolves; both halves run the same script. Only flips
    // state, so it is safe to call from the bridge's receive path (ISR context).
    void LeftTest_Start(uint8_t testId);

    // Runs on the key scanner thread, in place of scanAllKeys.
    void LeftTest_Tick(void);

#endif // __LEFT_TEST_H__

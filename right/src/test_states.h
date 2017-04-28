#ifndef __TEST_STATES__
#define __TEST_STATES__

// Typedefs:

    typedef struct {
        bool disableUsb;
        bool disableI2c;
        bool disableKeyMatrixScan;
        bool disableLedSdb;
        bool disableLedFetPwm;
        bool disableLedDriverPwm;
    } test_states_t;

// Variables:

    extern test_states_t TestStates;

#endif

#ifndef __ROUND_TRIP_TEST_H__
#define __ROUND_TRIP_TEST_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>

// Macros:

// Typedefs:

// Variables:

    extern uint16_t RoundTripTime;

// Functions:

    void RoundTripTest_Init();
    void RoundTripTest_Run();
    void RoundTripTest_Receive(const uint8_t* data, uint16_t len);

#endif

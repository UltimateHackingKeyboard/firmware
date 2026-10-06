#ifndef __TEST_STATISTICS_H__
#define __TEST_STATISTICS_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>

// Macros:

    // How often a healthy repeated test is allowed to fail, over the whole test: its timings are
    // set against random stress, so some failures are expected and only their number is evidence.
    #define TEST_SPURIOUS_FAILURE_RATE 0.2f

// Functions:

    // Confidence that `failed` failed repetitions out of `repetitions` are more than chance.
    float TestStatistics_RepeatedTestConfidence(uint16_t failed, uint16_t repetitions);

    // The repeated tests of a suite run, combined by Fisher's method.
    void TestStatistics_Reset(void);
    void TestStatistics_AddRepeatedTest(float confidence, bool failed);
    uint16_t TestStatistics_RepeatedTestCount(void);
    uint16_t TestStatistics_FailedRepeatedTestCount(void);
    float TestStatistics_SuiteConfidence(void);

    unsigned TestStatistics_AsPercent(float probability);

#endif

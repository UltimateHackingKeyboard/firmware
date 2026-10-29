#ifndef __TEST_STATISTICS_H__
#define __TEST_STATISTICS_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>

// Macros:

    // A repeated test fails once it is broken with at least this probability.
    #define TEST_FAIL_PROBABILITY 0.95

// Functions:

    // Probability that a test is broken, given `failed` >= 1 failed repetitions out of
    // `repetitions`, a healthy repetition failing with probability `rate`.
    double TestStatistics_BrokenProbability(uint16_t failed, uint16_t repetitions, double rate);

    // The repeated tests of a suite run.
    void TestStatistics_Reset(void);
    void TestStatistics_AddRepeatedTest(bool failed);
    uint16_t TestStatistics_RepeatedTestCount(void);
    uint16_t TestStatistics_FailedRepeatedTestCount(void);

    unsigned TestStatistics_AsPercent(double probability);

#endif

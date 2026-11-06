#include "test_statistics.h"

// A broken test fails at a rate anywhere in [TEST_BROKEN_RATE_MIN_FACTOR * its healthy rate,
// TEST_BROKEN_RATE_MAX], log-uniformly; TEST_BROKEN_PRIOR is how likely it is broken before any
// repetition ran.
#define TEST_BROKEN_PRIOR 0.5
#define TEST_BROKEN_RATE_MIN_FACTOR 2.0
#define TEST_BROKEN_RATE_MAX 0.5

// Repeated tests run in this suite pass, and how many of them failed.
static uint16_t statisticalCount = 0;
static uint16_t statisticalFailedCount = 0;

// P(X < k) for X ~ Binomial(n, t); the probability of exactly k is left in *pAtK.
static double binomialBelow(uint16_t k, uint16_t n, double t, double *pAtK) {
    double pExactly = 1.0;
    for (uint16_t i = 0; i < n; i++) {
        pExactly *= 1.0 - t;
    }
    double pBelow = 0.0;
    for (uint16_t i = 0; i < k && i < n; i++) {
        pBelow += pExactly;
        pExactly *= (double)(n - i) / (double)(i + 1) * t / (1.0 - t);
    }
    *pAtK = pExactly;
    return pBelow;
}

// Natural log without libm: the double's exponent, plus 2 * atanh((m - 1) / (m + 1)) for the
// mantissa m in [0.5, 1), whose series is accurate to ~1e-5 within four terms.
static double naturalLog(double x) {
    union {
        double f;
        uint64_t i;
    } u = { x };
    int32_t exponent = (int32_t)((u.i >> 52) & 0x7ff) - 1022;
    u.i = (u.i & 0x800fffffffffffffull) | (1022ull << 52);
    double t = (u.f - 1.0) / (u.f + 1.0);
    double t2 = t * t;
    double mantissaLog = 2.0 * t * (1.0 + t2 * (1.0 / 3 + t2 * (1.0 / 5 + t2 * (1.0 / 7))));
    return exponent * 0.6931471805599453 + mantissaLog;
}

// How likely `failed` failures out of `repetitions` are for a broken test: the binomial
// probability averaged over the broken rates. The log-uniform density's 1/t cancels a power of
// t, leaving a beta integral, which is a difference of binomial tails:
// (P(X >= k | max) - P(X >= k | min)) / (k * ln(max / min)).
static double brokenLikelihood(uint16_t failed, uint16_t repetitions, double rate) {
    double minRate = TEST_BROKEN_RATE_MIN_FACTOR * rate;
    double unused;
    double belowAtMin = binomialBelow(failed, repetitions, minRate, &unused);
    double belowAtMax = binomialBelow(failed, repetitions, TEST_BROKEN_RATE_MAX, &unused);
    return (belowAtMin - belowAtMax) / (failed * naturalLog(TEST_BROKEN_RATE_MAX / minRate));
}

unsigned TestStatistics_AsPercent(double probability) {
    return (unsigned)(probability * 100.0 + 0.5);
}

double TestStatistics_BrokenProbability(uint16_t failed, uint16_t repetitions, double rate) {
    double healthyLikelihood;
    binomialBelow(failed, repetitions, rate, &healthyLikelihood);
    double broken = TEST_BROKEN_PRIOR * brokenLikelihood(failed, repetitions, rate);
    double healthy = (1.0 - TEST_BROKEN_PRIOR) * healthyLikelihood;
    return broken / (broken + healthy);
}

void TestStatistics_Reset(void) {
    statisticalCount = 0;
    statisticalFailedCount = 0;
}

void TestStatistics_AddRepeatedTest(bool failed) {
    statisticalCount++;
    if (failed) {
        statisticalFailedCount++;
    }
}

uint16_t TestStatistics_RepeatedTestCount(void) {
    return statisticalCount;
}

uint16_t TestStatistics_FailedRepeatedTestCount(void) {
    return statisticalFailedCount;
}

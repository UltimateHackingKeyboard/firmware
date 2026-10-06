#include "test_statistics.h"

// Repeated tests run in this suite pass, how many of them failed, and their p-values
// combined (product and -ln of the product) for Fisher's method.
static uint16_t statisticalCount = 0;
static uint16_t statisticalFailedCount = 0;
static float statisticalPProduct = 1.0f;
static float statisticalNegLogP = 0.0f;

// -ln(1 - TEST_SPURIOUS_FAILURE_RATE) by its series r + r^2/2 + r^3/3 + ..., folded at compile
// time; five terms are accurate to ~1e-5 at 20%.
#define TEST_R TEST_SPURIOUS_FAILURE_RATE
#define TEST_SPURIOUS_FAILURE_NEG_LOG \
    (TEST_R + TEST_R*TEST_R/2 + TEST_R*TEST_R*TEST_R/3 + TEST_R*TEST_R*TEST_R*TEST_R/4 \
        + TEST_R*TEST_R*TEST_R*TEST_R*TEST_R/5)

// Confidence that `failed` failures out of `trials` are more than chance, each trial failing
// spuriously with probability `rate`: the probability of seeing fewer, P(X < failed) for
// X ~ Binomial(trials, rate).
static float confidenceSomethingIsWrong(uint16_t failed, uint16_t trials, float rate) {
    float pExactly = 1.0f;
    for (uint16_t i = 0; i < trials; i++) {
        pExactly *= 1.0f - rate;
    }
    float pFewer = 0.0f;
    for (uint16_t k = 0; k < failed && k < trials; k++) {
        pFewer += pExactly;
        pExactly *= (float)(trials - k) / (float)(k + 1) * rate / (1.0f - rate);
    }
    return pFewer;
}

// A repetition's own spurious rate, such that the whole test fails at TEST_SPURIOUS_FAILURE_RATE:
// 1 - (1 - rate)^(1/n), to first order in 1/n. Not via a pow approximation - the result is the
// small difference from 1, which a few percent of error in the power would swamp.
static float repetitionFailureRate(uint16_t repetitions) {
    return TEST_SPURIOUS_FAILURE_NEG_LOG / repetitions;
}

// Natural log without libm: the float's exponent, plus 2 * atanh((m - 1) / (m + 1)) for the
// mantissa m in [0.5, 1), whose series is accurate to ~1e-5 within four terms.
static float naturalLog(float x) {
    union {
        float f;
        uint32_t i;
    } u = { x };
    int32_t exponent = (int32_t)((u.i >> 23) & 0xff) - 126;
    u.i = (u.i & 0x807fffff) | (126u << 23);
    float t = (u.f - 1.0f) / (u.f + 1.0f);
    float t2 = t * t;
    float mantissaLog = 2.0f * t * (1.0f + t2 * (1.0f / 3 + t2 * (1.0f / 5 + t2 * (1.0f / 7))));
    return exponent * 0.6931472f + mantissaLog;
}

// Fisher's method: how likely a product of `count` healthy p-values is to come out as small
// as `product`, i.e. product * sum_{k < count} (-ln product)^k / k!, and one minus that.
static float combinedConfidence(float product, float negLogProduct, uint16_t count) {
    float tail = 0.0f;
    float term = 1.0f;
    for (uint16_t k = 0; k < count; k++) {
        tail += term;
        term *= negLogProduct / (float)(k + 1);
    }
    return 1.0f - product * tail;
}

unsigned TestStatistics_AsPercent(float probability) {
    return (unsigned)(probability * 100.0f + 0.5f);
}

void TestStatistics_Reset(void) {
    statisticalCount = 0;
    statisticalFailedCount = 0;
    statisticalPProduct = 1.0f;
    statisticalNegLogP = 0.0f;
}

float TestStatistics_RepeatedTestConfidence(uint16_t failed, uint16_t repetitions) {
    return confidenceSomethingIsWrong(failed, repetitions, repetitionFailureRate(repetitions));
}

void TestStatistics_AddRepeatedTest(float confidence, bool failed) {
    // Floored, so a near-certain failure does not underflow the product to nothing.
    float pValue = 1.0f - confidence;
    if (pValue < 1e-30f) {
        pValue = 1e-30f;
    }
    statisticalCount++;
    statisticalPProduct *= pValue;
    statisticalNegLogP -= naturalLog(pValue);
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

float TestStatistics_SuiteConfidence(void) {
    return combinedConfidence(statisticalPProduct, statisticalNegLogP, statisticalCount);
}

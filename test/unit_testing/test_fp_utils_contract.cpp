#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "fp_utils.h"

#define ASSERT_FP_CONTRACT(cond, msg) do { if (!(cond)) { std::cerr << "[FAIL] test_fp_utils_contract: " << msg << " (" << #cond << ")" << std::endl; std::exit(EXIT_FAILURE); } } while (0)

static void checkFormat(uint8_t expBits, uint8_t mantBits,
                        uint32_t expectedBias,
                        uint32_t expectedOne,
                        uint32_t expectedPosInf,
                        uint32_t expectedNegInf,
                        double expectedMin,
                        double expectedMax,
                        double maxTolerance) {
    FPUtils u(expBits, mantBits);
    ASSERT_FP_CONTRACT(u.getSizeExponent() == expBits, "exponent size getter mismatch");
    ASSERT_FP_CONTRACT(u.getSizeMantissa() == mantBits, "mantissa size getter mismatch");
    ASSERT_FP_CONTRACT(u.getTotalBits() == static_cast<uint8_t>(1 + expBits + mantBits), "total bit width mismatch");
    ASSERT_FP_CONTRACT(u.getBias() == expectedBias, "bias mismatch");

    uint32_t one = u.pack(false, expectedBias, 0);
    ASSERT_FP_CONTRACT(one == expectedOne, "1.0 packed pattern mismatch");
    ASSERT_FP_CONTRACT(!u.getSign(one), "1.0 sign mismatch");
    ASSERT_FP_CONTRACT(u.getExponent(one) == expectedBias, "1.0 exponent extraction mismatch");
    ASSERT_FP_CONTRACT(u.getMantissa(one) == 0, "1.0 mantissa extraction mismatch");

    uint32_t negZero = u.pack(true, 0, 0);
    ASSERT_FP_CONTRACT(u.isZero(0), "+0 should be zero");
    ASSERT_FP_CONTRACT(u.isZero(negZero), "-0 should be zero");
    ASSERT_FP_CONTRACT(u.getSign(negZero), "-0 sign should be preserved");

    ASSERT_FP_CONTRACT(u.getPositiveInf() == expectedPosInf, "+inf pattern mismatch");
    ASSERT_FP_CONTRACT(u.getNegativeInf() == expectedNegInf, "-inf pattern mismatch");
    ASSERT_FP_CONTRACT(u.isInf(expectedPosInf), "+inf detection mismatch");
    ASSERT_FP_CONTRACT(u.isInf(expectedNegInf), "-inf detection mismatch");
    ASSERT_FP_CONTRACT(!u.isNaN(expectedPosInf), "inf falsely detected as NaN");

    uint32_t nan = u.getNaN();
    ASSERT_FP_CONTRACT(u.isNaN(nan), "NaN detection mismatch");
    ASSERT_FP_CONTRACT(!u.isInf(nan), "NaN falsely detected as infinity");
    ASSERT_FP_CONTRACT(u.getMantissa(nan) != 0, "NaN mantissa must be non-zero");

    ASSERT_FP_CONTRACT(std::fabs(u.getMin() - expectedMin) <= std::fabs(expectedMin) * 1e-12 + 1e-45, "getMin mismatch");
    ASSERT_FP_CONTRACT(std::fabs(u.getMax() - expectedMax) <= maxTolerance, "getMax mismatch");

    uint32_t allOnesMant = mantBits >= 32 ? 0xFFFFFFFFu : ((1u << mantBits) - 1u);
    uint32_t maxExp = expBits >= 32 ? 0xFFFFFFFFu : ((1u << expBits) - 1u);
    uint32_t packed = u.pack(true, maxExp + 17u, allOnesMant + 123u);
    ASSERT_FP_CONTRACT(u.getSign(packed), "pack must preserve sign bit");
    ASSERT_FP_CONTRACT(u.getExponent(packed) == maxExp, "pack must mask exponent width");
    ASSERT_FP_CONTRACT(u.getMantissa(packed) == allOnesMant, "pack must mask mantissa width");
}

int main() {
    std::cout << "--- Starting FPUtils Contract Matrix Tests ---" << std::endl;

    checkFormat(8, 23, 127, 0x3F800000u, 0x7F800000u, 0xFF800000u,
                std::ldexp(1.0, -126), 3.4028234663852886e38, 1e31);
    checkFormat(5, 10, 15, 0x3C00u, 0x7C00u, 0xFC00u,
                std::ldexp(1.0, -14), 65504.0, 0.1);
    checkFormat(4, 8, 7, 0x0700u, 0x0F00u, 0x1F00u,
                std::ldexp(1.0, -6), 255.5, 1e-9);

    std::cout << "[SUCCESS] FPUtils Contract Matrix Tests passed!" << std::endl;
    return 0;
}

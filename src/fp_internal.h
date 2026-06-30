#ifndef FP_INTERNAL_H
#define FP_INTERNAL_H

#include <stdint.h>

#include "fp_utils.h"

namespace fp_internal {

enum FloatClass {
    CLASS_ZERO,
    CLASS_NORMAL,
    CLASS_INF,
    CLASS_NAN
};

struct UnpackedFloat {
    bool sign;
    FloatClass cls;
    int64_t exponent;
    uint64_t significand;
};

struct ShiftRightResult {
    uint64_t value;
    bool sticky;
};

UnpackedFloat unpackBits(uint32_t bits, const FPUtils& utils);

uint32_t makeZero(bool sign, const FPUtils& utils);
uint32_t makeInf(bool sign, const FPUtils& utils);
uint32_t makeNaN(const FPUtils& utils);

uint32_t packUnpackedSimple(const UnpackedFloat& value, const FPUtils& utils);

void setFlagsFromResult(
    uint32_t bits,
    const FPUtils& utils,
    bool overflowValue,
    bool underflowValue,
    bool inexactValue,
    bool& zero,
    bool& sign,
    bool& overflow,
    bool& underflow,
    bool& inexact,
    bool& nan
);

int compareMagnitude(const UnpackedFloat& left, const UnpackedFloat& right);

ShiftRightResult shiftRightWithSticky(uint64_t value, uint32_t shift);

uint8_t bitLength64(uint64_t value);
uint64_t hiddenBit(const FPUtils& utils);
uint64_t mantissaMask(const FPUtils& utils);
uint32_t maxExponentField(const FPUtils& utils);

} // namespace fp_internal

#endif

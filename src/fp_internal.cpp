#include "fp_internal.h"

namespace fp_internal {

uint64_t hiddenBit(const FPUtils& utils) {
    uint8_t mantissaBits = utils.getSizeMantissa();

    if (mantissaBits >= 64) {
        return 0;
    }

    return 1ULL << mantissaBits;
}

uint64_t mantissaMask(const FPUtils& utils) {
    uint8_t mantissaBits = utils.getSizeMantissa();

    if (mantissaBits >= 64) {
        return UINT64_MAX;
    }

    if (mantissaBits == 0) {
        return 0;
    }

    return (1ULL << mantissaBits) - 1ULL;
}

uint32_t maxExponentField(const FPUtils& utils) {
    uint8_t exponentBits = utils.getSizeExponent();

    if (exponentBits >= 32) {
        return UINT32_MAX;
    }

    return static_cast<uint32_t>((1ULL << exponentBits) - 1ULL);
}

UnpackedFloat unpackBits(uint32_t bits, const FPUtils& utils) {
    UnpackedFloat value;
    value.sign = utils.getSign(bits);
    value.cls = CLASS_ZERO;
    value.exponent = 0;
    value.significand = 0;

    uint32_t exponentField = utils.getExponent(bits);
    uint32_t mantissaField = utils.getMantissa(bits);
    uint32_t maxExponent = maxExponentField(utils);

    if (exponentField == 0 && mantissaField == 0) {
        value.cls = CLASS_ZERO;
        return value;
    }

    if (exponentField == 0 && mantissaField != 0) {
        // Subnormals are not required in the project, so keep the sign and flush to zero.
        value.cls = CLASS_ZERO;
        return value;
    }

    if (exponentField == maxExponent && mantissaField == 0) {
        value.cls = CLASS_INF;
        return value;
    }

    if (exponentField == maxExponent && mantissaField != 0) {
        value.cls = CLASS_NAN;
        value.sign = false;
        return value;
    }

    value.cls = CLASS_NORMAL;
    value.exponent =
        static_cast<int64_t>(exponentField) - static_cast<int64_t>(utils.getBias());
    value.significand = hiddenBit(utils) | static_cast<uint64_t>(mantissaField);

    return value;
}

uint32_t makeZero(bool sign, const FPUtils& utils) {
    if (sign) {
        return utils.getNegativeZero();
    }

    return utils.getPositiveZero();
}

uint32_t makeInf(bool sign, const FPUtils& utils) {
    if (sign) {
        return utils.getNegativeInf();
    }

    return utils.getPositiveInf();
}

uint32_t makeNaN(const FPUtils& utils) {
    return utils.getNaN();
}

uint32_t packUnpackedSimple(const UnpackedFloat& value, const FPUtils& utils) {
    if (value.cls == CLASS_ZERO) {
        return makeZero(value.sign, utils);
    }

    if (value.cls == CLASS_INF) {
        return makeInf(value.sign, utils);
    }

    if (value.cls == CLASS_NAN) {
        return makeNaN(utils);
    }

    int64_t exponentField =
        value.exponent + static_cast<int64_t>(utils.getBias());

    if (exponentField <= 0) {
        return makeZero(value.sign, utils);
    }

    if (exponentField >= static_cast<int64_t>(maxExponentField(utils))) {
        return makeInf(value.sign, utils);
    }

    uint32_t mantissa =
        static_cast<uint32_t>(value.significand & mantissaMask(utils));

    return utils.pack(
        value.sign,
        static_cast<uint32_t>(exponentField),
        mantissa
    );
}

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
) {
    zero = utils.isZero(bits);
    sign = utils.getSign(bits);
    overflow = overflowValue;
    underflow = underflowValue;
    inexact = inexactValue;
    nan = utils.isNaN(bits);
}

int compareMagnitude(const UnpackedFloat& left, const UnpackedFloat& right) {
    if (left.cls == CLASS_NAN || right.cls == CLASS_NAN) {
        return 0;
    }

    if (left.cls == CLASS_INF && right.cls == CLASS_INF) {
        return 0;
    }

    if (left.cls == CLASS_INF) {
        return 1;
    }

    if (right.cls == CLASS_INF) {
        return -1;
    }

    if (left.cls == CLASS_ZERO && right.cls == CLASS_ZERO) {
        return 0;
    }

    if (left.cls == CLASS_ZERO) {
        return -1;
    }

    if (right.cls == CLASS_ZERO) {
        return 1;
    }

    if (left.exponent < right.exponent) {
        return -1;
    }

    if (left.exponent > right.exponent) {
        return 1;
    }

    if (left.significand < right.significand) {
        return -1;
    }

    if (left.significand > right.significand) {
        return 1;
    }

    return 0;
}

ShiftRightResult shiftRightWithSticky(uint64_t value, uint32_t shift) {
    ShiftRightResult result;
    result.value = value;
    result.sticky = false;

    if (shift == 0) {
        return result;
    }

    if (shift >= 64) {
        result.value = 0;
        result.sticky = value != 0;
        return result;
    }

    uint64_t discardedMask = (1ULL << shift) - 1ULL;
    result.value = value >> shift;
    result.sticky = (value & discardedMask) != 0;

    return result;
}

uint8_t bitLength64(uint64_t value) {
    uint8_t length = 0;

    while (value != 0) {
        length++;
        value = value >> 1;
    }

    return length;
}

} // namespace fp_internal

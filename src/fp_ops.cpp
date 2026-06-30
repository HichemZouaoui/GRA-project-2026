#include "fp_ops.h"
#include "fma_core.h"
#include "fp_internal.h"
#include "rounder.h"

static const uint8_t OP_FADD = 8;
static const uint8_t OP_FSUB = 9;
static const uint8_t OP_FMUL = 10;
static const uint8_t OP_FMIN = 13;
static const uint8_t OP_FMAX = 14;
static const uint8_t OP_FMA = 15;

static uint32_t finalResult(
    uint32_t result,
    const FPUtils& utils,
    bool& zero,
    bool& sign,
    bool& overflow,
    bool& underflow,
    bool& inexact,
    bool& nan
) {
    zero = utils.isZero(result);
    sign = utils.getSign(result);
    overflow = false;
    underflow = false;
    inexact = false;
    nan = utils.isNaN(result);

    return result;
}

static int compareByValue(
    uint32_t leftBits,
    uint32_t rightBits,
    const FPUtils& utils
) {
    fp_internal::UnpackedFloat left = fp_internal::unpackBits(leftBits, utils);
    fp_internal::UnpackedFloat right = fp_internal::unpackBits(rightBits, utils);

    if (left.sign != right.sign) {
        if (left.sign) {
            return -1;
        }
        return 1;
    }

    int magnitudeCompare = fp_internal::compareMagnitude(left, right);

    if (left.sign) {
        return -magnitudeCompare;
    }

    return magnitudeCompare;
}

static uint32_t flipSignBit(uint32_t bits, const FPUtils& utils) {
    uint32_t signMask = 1U << (utils.getSizeExponent() + utils.getSizeMantissa());
    return bits ^ signMask;
}

static bool handleAddSpecialCases(
    uint32_t leftBits,
    uint32_t rightBits,
    const FPUtils& utils,
    uint8_t roundMode,
    uint32_t& result
) {
    fp_internal::UnpackedFloat left = fp_internal::unpackBits(leftBits, utils);
    fp_internal::UnpackedFloat right = fp_internal::unpackBits(rightBits, utils);

    if (left.cls == fp_internal::CLASS_NAN || right.cls == fp_internal::CLASS_NAN) {
        result = fp_internal::makeNaN(utils);
        return true;
    }

    if (left.cls == fp_internal::CLASS_INF && right.cls == fp_internal::CLASS_INF) {
        if (left.sign == right.sign) {
            result = fp_internal::makeInf(left.sign, utils);
        }
        else {
            result = fp_internal::makeNaN(utils);
        }
        return true;
    }

    if (left.cls == fp_internal::CLASS_INF) {
        result = fp_internal::makeInf(left.sign, utils);
        return true;
    }

    if (right.cls == fp_internal::CLASS_INF) {
        result = fp_internal::makeInf(right.sign, utils);
        return true;
    }

    if (left.cls == fp_internal::CLASS_ZERO && right.cls == fp_internal::CLASS_ZERO) {
        if (left.sign == right.sign) {
            result = fp_internal::makeZero(left.sign, utils);
        }
        else if (roundMode == 4) {
            result = fp_internal::makeZero(true, utils);
        }
        else {
            result = fp_internal::makeZero(false, utils);
        }
        return true;
    }

    if (left.cls == fp_internal::CLASS_ZERO && right.cls == fp_internal::CLASS_NORMAL) {
        result = rightBits;
        return true;
    }

    if (left.cls == fp_internal::CLASS_NORMAL && right.cls == fp_internal::CLASS_ZERO) {
        result = leftBits;
        return true;
    }

    return false;
}

static uint64_t shiftRightOneWithSticky(uint64_t value) {
    bool droppedBit = (value & 1ULL) != 0;
    uint64_t shifted = value >> 1;

    if (droppedBit) {
        shifted = shifted | 1ULL;
    }

    return shifted;
}

static uint64_t alignSignificand(uint64_t significandWithExtra, uint32_t shift) {
    fp_internal::ShiftRightResult shifted =
        fp_internal::shiftRightWithSticky(significandWithExtra, shift);

    if (shifted.sticky) {
        shifted.value = shifted.value | 1ULL;
    }

    return shifted.value;
}

static uint32_t finishFiniteAddResult(
    bool resultSign,
    int64_t resultExponent,
    uint64_t significandWithExtra,
    const FPUtils& utils,
    uint8_t roundMode,
    bool& zero,
    bool& sign,
    bool& overflow,
    bool& underflow,
    bool& inexact,
    bool& nan
) {
    static const uint8_t EXTRA_BITS = 3;

    if (significandWithExtra == 0) {
        uint32_t zeroResult = fp_internal::makeZero(roundMode == 4, utils);
        return finalResult(
            zeroResult,
            utils,
            zero,
            sign,
            overflow,
            underflow,
            inexact,
            nan
        );
    }

    uint64_t hiddenBit = fp_internal::hiddenBit(utils);
    uint64_t normalBitWithExtra = hiddenBit << EXTRA_BITS;
    uint64_t carryBitWithExtra = hiddenBit << (EXTRA_BITS + 1);

    while (significandWithExtra >= carryBitWithExtra) {
        significandWithExtra = shiftRightOneWithSticky(significandWithExtra);
        resultExponent++;
    }

    while (significandWithExtra < normalBitWithExtra) {
        significandWithExtra = significandWithExtra << 1;
        resultExponent--;
    }

    int64_t exponentField =
        resultExponent + static_cast<int64_t>(utils.getBias());

    if (exponentField <= 0) {
        uint32_t zeroResult = fp_internal::makeZero(resultSign, utils);

        zero = true;
        sign = resultSign;
        overflow = false;
        underflow = true;
        inexact = true;
        nan = false;

        return zeroResult;
    }

    bool localInexact = false;
    uint32_t roundedSignificand = Rounder::roundMantissa(
        significandWithExtra,
        EXTRA_BITS,
        roundMode,
        resultSign,
        localInexact
    );

    if (roundedSignificand >= static_cast<uint32_t>(hiddenBit << 1)) {
        roundedSignificand = roundedSignificand >> 1;
        resultExponent++;
        exponentField = resultExponent + static_cast<int64_t>(utils.getBias());
    }

    if (exponentField >= static_cast<int64_t>(fp_internal::maxExponentField(utils))) {
        uint32_t infResult = fp_internal::makeInf(resultSign, utils);

        zero = false;
        sign = resultSign;
        overflow = true;
        underflow = false;
        inexact = true;
        nan = false;

        return infResult;
    }

    if (exponentField <= 0) {
        uint32_t zeroResult = fp_internal::makeZero(resultSign, utils);

        zero = true;
        sign = resultSign;
        overflow = false;
        underflow = true;
        inexact = true;
        nan = false;

        return zeroResult;
    }

    uint32_t mantissa =
        static_cast<uint32_t>(roundedSignificand & fp_internal::mantissaMask(utils));
    uint32_t result = utils.pack(
        resultSign,
        static_cast<uint32_t>(exponentField),
        mantissa
    );

    zero = false;
    sign = resultSign;
    overflow = false;
    underflow = false;
    inexact = localInexact;
    nan = false;

    return result;
}

static uint32_t computeFiniteAdd(
    uint32_t leftBits,
    uint32_t rightBits,
    const FPUtils& utils,
    uint8_t roundMode,
    bool& zero,
    bool& sign,
    bool& overflow,
    bool& underflow,
    bool& inexact,
    bool& nan
) {
    static const uint8_t EXTRA_BITS = 3;

    fp_internal::UnpackedFloat left = fp_internal::unpackBits(leftBits, utils);
    fp_internal::UnpackedFloat right = fp_internal::unpackBits(rightBits, utils);

    int64_t resultExponent = left.exponent;
    uint64_t leftSignificand = left.significand << EXTRA_BITS;
    uint64_t rightSignificand = right.significand << EXTRA_BITS;

    if (left.exponent > right.exponent) {
        uint32_t shift = static_cast<uint32_t>(left.exponent - right.exponent);
        rightSignificand = alignSignificand(rightSignificand, shift);
        resultExponent = left.exponent;
    }
    else if (right.exponent > left.exponent) {
        uint32_t shift = static_cast<uint32_t>(right.exponent - left.exponent);
        leftSignificand = alignSignificand(leftSignificand, shift);
        resultExponent = right.exponent;
    }

    bool resultSign = false;
    uint64_t resultSignificand = 0;

    if (left.sign == right.sign) {
        resultSign = left.sign;
        resultSignificand = leftSignificand + rightSignificand;
    }
    else {
        int magnitudeCompare = fp_internal::compareMagnitude(left, right);

        if (magnitudeCompare == 0) {
            uint32_t zeroResult = fp_internal::makeZero(roundMode == 4, utils);
            return finalResult(
                zeroResult,
                utils,
                zero,
                sign,
                overflow,
                underflow,
                inexact,
                nan
            );
        }

        if (magnitudeCompare > 0) {
            resultSign = left.sign;
            resultSignificand = leftSignificand - rightSignificand;
        }
        else {
            resultSign = right.sign;
            resultSignificand = rightSignificand - leftSignificand;
        }
    }

    return finishFiniteAddResult(
        resultSign,
        resultExponent,
        resultSignificand,
        utils,
        roundMode,
        zero,
        sign,
        overflow,
        underflow,
        inexact,
        nan
    );
}

static bool handleMulSpecialCases(
    uint32_t leftBits,
    uint32_t rightBits,
    const FPUtils& utils,
    uint32_t& result
) {
    fp_internal::UnpackedFloat left = fp_internal::unpackBits(leftBits, utils);
    fp_internal::UnpackedFloat right = fp_internal::unpackBits(rightBits, utils);
    bool resultSign = left.sign != right.sign;

    if (left.cls == fp_internal::CLASS_ZERO && right.cls == fp_internal::CLASS_NAN) {
        result = fp_internal::makeZero(left.sign, utils);
        return true;
    }

    if (left.cls == fp_internal::CLASS_NAN && right.cls == fp_internal::CLASS_ZERO) {
        result = fp_internal::makeZero(right.sign, utils);
        return true;
    }

    if (left.cls == fp_internal::CLASS_NAN || right.cls == fp_internal::CLASS_NAN) {
        result = fp_internal::makeNaN(utils);
        return true;
    }

    if (left.cls == fp_internal::CLASS_INF && right.cls == fp_internal::CLASS_ZERO) {
        result = fp_internal::makeNaN(utils);
        return true;
    }

    if (left.cls == fp_internal::CLASS_ZERO && right.cls == fp_internal::CLASS_INF) {
        result = fp_internal::makeNaN(utils);
        return true;
    }

    if (left.cls == fp_internal::CLASS_INF || right.cls == fp_internal::CLASS_INF) {
        result = fp_internal::makeInf(resultSign, utils);
        return true;
    }

    if (left.cls == fp_internal::CLASS_ZERO || right.cls == fp_internal::CLASS_ZERO) {
        result = fp_internal::makeZero(resultSign, utils);
        return true;
    }

    return false;
}

static uint64_t productToSignificandWithExtra(uint64_t product, const FPUtils& utils) {
    static const uint8_t EXTRA_BITS = 3;

    uint8_t mantissaBits = utils.getSizeMantissa();

    if (mantissaBits >= EXTRA_BITS) {
        uint32_t shift = mantissaBits - EXTRA_BITS;
        fp_internal::ShiftRightResult shifted =
            fp_internal::shiftRightWithSticky(product, shift);

        if (shifted.sticky) {
            shifted.value = shifted.value | 1ULL;
        }

        return shifted.value;
    }

    return product << (EXTRA_BITS - mantissaBits);
}

static uint32_t computeFiniteMul(
    uint32_t leftBits,
    uint32_t rightBits,
    const FPUtils& utils,
    uint8_t roundMode,
    bool& zero,
    bool& sign,
    bool& overflow,
    bool& underflow,
    bool& inexact,
    bool& nan
) {
    fp_internal::UnpackedFloat left = fp_internal::unpackBits(leftBits, utils);
    fp_internal::UnpackedFloat right = fp_internal::unpackBits(rightBits, utils);

    bool resultSign = left.sign != right.sign;
    int64_t resultExponent = left.exponent + right.exponent;

    uint64_t product = left.significand * right.significand;
    uint64_t productWithExtra = productToSignificandWithExtra(product, utils);

    return finishFiniteAddResult(
        resultSign,
        resultExponent,
        productWithExtra,
        utils,
        roundMode,
        zero,
        sign,
        overflow,
        underflow,
        inexact,
        nan
    );
}

uint32_t FPOps::execute(
    uint8_t op,
    uint32_t r1,
    uint32_t r2,
    uint32_t r3,
    const FPUtils& utils,
    uint8_t roundMode,
    bool& zero,
    bool& sign,
    bool& overflow,
    bool& underflow,
    bool& inexact,
    bool& nan
) {
    switch (op) {
        case OP_FADD:
        {
            uint32_t specialResult = 0;
            if (handleAddSpecialCases(r1, r2, utils, roundMode, specialResult)) {
                return finalResult(
                    specialResult,
                    utils,
                    zero,
                    sign,
                    overflow,
                    underflow,
                    inexact,
                    nan
                );
            }

            return computeFiniteAdd(
                r1,
                r2,
                utils,
                roundMode,
                zero,
                sign,
                overflow,
                underflow,
                inexact,
                nan
            );
        }
        
        case OP_FSUB:
        {
            uint32_t specialResult = 0;
            uint32_t negativeR2 = flipSignBit(r2, utils);
            if (handleAddSpecialCases(r1, negativeR2, utils, roundMode, specialResult)) {
                return finalResult(
                    specialResult,
                    utils,
                    zero,
                    sign,
                    overflow,
                    underflow,
                    inexact,
                    nan
                );
            }

            return computeFiniteAdd(
                r1,
                negativeR2,
                utils,
                roundMode,
                zero,
                sign,
                overflow,
                underflow,
                inexact,
                nan
            );
        }

        case OP_FMUL:
        {
            uint32_t specialResult = 0;
            if (handleMulSpecialCases(r1, r2, utils, specialResult)) {
                return finalResult(
                    specialResult,
                    utils,
                    zero,
                    sign,
                    overflow,
                    underflow,
                    inexact,
                    nan
                );
            }

            return computeFiniteMul(
                r1,
                r2,
                utils,
                roundMode,
                zero,
                sign,
                overflow,
                underflow,
                inexact,
                nan
            );
        }

        case OP_FMIN:
        if (utils.isNaN(r1) || utils.isNaN(r2)) {
            return finalResult(
                utils.getNaN(),
                utils,
                zero,
                sign,
                overflow,
                underflow,
                inexact,
                nan
            );
        }
                
        if (utils.isZero(r1) && utils.isZero(r2)) {
            if (utils.getSign(r1) || utils.getSign(r2)) {
                return finalResult(
                    utils.getNegativeZero(),
                    utils,
                    zero,
                    sign,
                    overflow,
                    underflow,
                    inexact,
                    nan
                );
            }

            return finalResult(
                utils.getPositiveZero(),
                utils,
                zero,
                sign,
                overflow,
                underflow,
                inexact,
                nan
            );
        }

        if (compareByValue(r1, r2, utils) <= 0) {
            return finalResult(
                r1, 
                utils, 
                zero, 
                sign, 
                overflow, 
                underflow, 
                inexact, 
                nan
            );
        }
        return finalResult(
            r2,
            utils,
            zero,
            sign,
            overflow,
            underflow,
            inexact,
            nan
        );
    
        case OP_FMAX:
        if (utils.isNaN(r1) || utils.isNaN(r2)) {
            return finalResult(
                utils.getNaN(),
                utils,
                zero,
                sign,
                overflow,
                underflow,
                inexact,
                nan
            );
        }

        if (utils.isZero(r1) && utils.isZero(r2)) {
            if (utils.getSign(r1) && utils.getSign(r2)) {
                return finalResult(
                    utils.getNegativeZero(),
                    utils,
                    zero,
                    sign,
                    overflow,
                    underflow,
                    inexact,
                    nan
                );
            }

            return finalResult(
                utils.getPositiveZero(),
                utils,
                zero,
                sign,
                overflow,
                underflow,
                inexact,
                nan
            );
        }

        if (compareByValue(r1, r2, utils) >= 0) {
            return finalResult(
                r1,
                utils,
                zero,
                sign,
                overflow,
                underflow,
                inexact,
                nan
            );
        }
        return finalResult(
                r2,
                utils,
                zero,
                sign,
                overflow,
                underflow,
                inexact,
                nan
            );
        
        case OP_FMA:
        return FMACore::executeFMA(
            r1, 
            r2, 
            r3, 
            utils, 
            roundMode,
            zero, 
            sign, 
            overflow, 
            underflow, 
            inexact,
            nan);

        default:
        return finalResult(
            utils.getNaN(),
            utils,
            zero,
            sign,
            overflow,
            underflow,
            inexact,
            nan
        );
    }
}

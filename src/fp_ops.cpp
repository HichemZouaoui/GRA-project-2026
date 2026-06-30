#include "fp_ops.h"
#include "fma_core.h"
#include "fp_internal.h"

#include <cmath>


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

static uint32_t finalComputedResult(
    long double exactResult,
    const FPUtils& utils,
    uint8_t roundMode,
    bool& zero,
    bool& sign,
    bool& overflow,
    bool& underflow,
    bool& inexact,
    bool& nan
) {
    bool localInexact = false;

    long double absResult = std::fabs(exactResult);
    underflow = 
        !std::isnan(exactResult) &&
        !std::isinf(exactResult) &&
        absResult != 0.0L &&
        absResult < static_cast<long double>(utils.getMin());

    uint32_t result = utils.encode(exactResult, roundMode, &localInexact);

    zero = utils.isZero(result);
    sign = utils.getSign(result);
    overflow = 
    !std::isnan(exactResult) && !std::isinf(exactResult) && utils.isInf(result);

    inexact = localInexact;
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

            // Finite normal arithmetic is still the old path for now.
            long double a = utils.decode(r1);
            long double b = utils.decode(r2);

            return finalComputedResult(
                a+b,
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

            // Finite normal arithmetic is still the old path for now.
            long double a = utils.decode(r1);
            long double b = utils.decode(r2);

            return finalComputedResult(
                a-b,
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
        if (utils.isZero(r1) && utils.isNaN(r2)) {
            if(utils.getSign(r1)) {
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

        if (utils.isNaN(r1) && utils.isZero(r2)) {
            if (utils.getSign(r2)) {
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

        {
            long double a = utils.decode(r1);
            long double b = utils.decode(r2);

            return finalComputedResult(
                        a*b,
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

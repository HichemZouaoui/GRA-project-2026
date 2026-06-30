#include "fma_core.h"
#include "fp_internal.h"

#include <cmath>
#include <cstdint>

static uint32_t finalFMAResult(
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

static uint32_t addZeroSigns(
    bool leftSign,
    bool rightSign,
    const FPUtils& utils,
    uint8_t roundMode
) {
    if (leftSign == rightSign) {
        return fp_internal::makeZero(leftSign, utils);
    }

    if (roundMode == 4) {
        return fp_internal::makeZero(true, utils);
    }

    return fp_internal::makeZero(false, utils);
}

static bool handleFMASpecialCases(
    uint32_t addBits,
    uint32_t mulLeftBits,
    uint32_t mulRightBits,
    const FPUtils& utils,
    uint8_t roundMode,
    uint32_t& result
) {
    fp_internal::UnpackedFloat add = fp_internal::unpackBits(addBits, utils);
    fp_internal::UnpackedFloat mulLeft = fp_internal::unpackBits(mulLeftBits, utils);
    fp_internal::UnpackedFloat mulRight = fp_internal::unpackBits(mulRightBits, utils);

    bool productSign = mulLeft.sign != mulRight.sign;
    bool productIsZero =
        mulLeft.cls == fp_internal::CLASS_ZERO ||
        mulRight.cls == fp_internal::CLASS_ZERO;
    bool productIsInf =
        mulLeft.cls == fp_internal::CLASS_INF ||
        mulRight.cls == fp_internal::CLASS_INF;

    if (add.cls == fp_internal::CLASS_NAN ||
        mulLeft.cls == fp_internal::CLASS_NAN ||
        mulRight.cls == fp_internal::CLASS_NAN) {
        result = fp_internal::makeNaN(utils);
        return true;
    }

    if (productIsInf && productIsZero) {
        result = fp_internal::makeNaN(utils);
        return true;
    }

    if (add.cls == fp_internal::CLASS_INF && productIsInf) {
        if (add.sign != productSign) {
            result = fp_internal::makeNaN(utils);
        }
        else {
            result = fp_internal::makeInf(add.sign, utils);
        }
        return true;
    }

    if (add.cls == fp_internal::CLASS_INF) {
        result = fp_internal::makeInf(add.sign, utils);
        return true;
    }

    if (productIsInf) {
        result = fp_internal::makeInf(productSign, utils);
        return true;
    }

    if (productIsZero) {
        if (add.cls == fp_internal::CLASS_ZERO) {
            result = addZeroSigns(add.sign, productSign, utils, roundMode);
        }
        else {
            result = addBits;
        }
        return true;
    }

    return false;
}

uint32_t FMACore::executeFMA(
    uint32_t r1,
    uint32_t r2,
    uint32_t r3,
    const FPUtils &utils, 
    uint8_t roundMode, 
    bool &zero, 
    bool &sign, 
    bool &overflow, 
    bool &underflow, 
    bool &inexact, 
    bool &nan) {
        uint32_t specialResult = 0;

        if (handleFMASpecialCases(r1, r2, r3, utils, roundMode, specialResult)) {
            return finalFMAResult(
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

        bool localInexact = false;

        long double a = utils.decode(r1);
        long double b = utils.decode(r2);
        long double c = utils.decode(r3);

        long double exactResult = a + (b * c);
        long double absResult = std::fabs(exactResult);

        underflow = 
        !std::isnan(exactResult) && !std::isinf(exactResult) &&
        absResult != 0.0L && absResult < static_cast<long double>(utils.getMin());

        uint32_t result = utils.encode(exactResult, roundMode, &localInexact);

        zero = utils.isZero(result);
        sign = utils.getSign(result);
        overflow = 
        !std::isnan(exactResult) && !std::isinf(exactResult) &&
        utils.isInf(result);

        inexact = localInexact;
        nan = utils.isNaN(result);

        return result;
    }

#include "fma_core.h"
#include "fp_internal.h"
#include "rounder.h"

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

static uint64_t shiftRightOneWithSticky(uint64_t value) {
    bool droppedBit = (value & 1ULL) != 0;
    uint64_t shifted = value >> 1;

    if (droppedBit) {
        shifted = shifted | 1ULL;
    }

    return shifted;
}

static uint64_t shiftRightWithStickyBit(uint64_t value, uint32_t shift) {
    fp_internal::ShiftRightResult shifted =
        fp_internal::shiftRightWithSticky(value, shift);

    if (shifted.sticky) {
        shifted.value = shifted.value | 1ULL;
    }

    return shifted.value;
}

static uint64_t shiftToLsbExponent(
    uint64_t value,
    int64_t oldLsbExponent,
    int64_t newLsbExponent
) {
    int64_t shift = oldLsbExponent - newLsbExponent;

    if (value == 0) {
        return 0;
    }

    if (shift > 0) {
        if (shift >= 64) {
            return 0;
        }

        return value << static_cast<uint32_t>(shift);
    }

    if (shift < 0) {
        int64_t rightShift = -shift;

        if (rightShift >= 64) {
            return 1;
        }

        return shiftRightWithStickyBit(value, static_cast<uint32_t>(rightShift));
    }

    return value;
}

static uint64_t moveTopBitToFinalPlace(
    uint64_t value,
    uint8_t oldTopBit,
    uint8_t newTopBit
) {
    if (oldTopBit > newTopBit) {
        uint32_t shift = oldTopBit - newTopBit;
        return shiftRightWithStickyBit(value, shift);
    }

    if (oldTopBit < newTopBit) {
        uint32_t shift = newTopBit - oldTopBit;
        return value << shift;
    }

    return value;
}

static uint32_t finishFiniteFMAResult(
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
        return finalFMAResult(
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

static uint32_t computeFiniteFMA(
    uint32_t addBits,
    uint32_t mulLeftBits,
    uint32_t mulRightBits,
    const FPUtils& utils,
    uint8_t roundMode,
    bool& zero,
    bool& sign,
    bool& overflow,
    bool& underflow,
    bool& inexact,
    bool& nan
) {
    fp_internal::UnpackedFloat add = fp_internal::unpackBits(addBits, utils);
    fp_internal::UnpackedFloat mulLeft = fp_internal::unpackBits(mulLeftBits, utils);
    fp_internal::UnpackedFloat mulRight = fp_internal::unpackBits(mulRightBits, utils);

    uint8_t mantissaBits = utils.getSizeMantissa();
    uint8_t productScale = static_cast<uint8_t>(mantissaBits * 2U);
    uint8_t workTopBit = static_cast<uint8_t>(productScale + 2U);

    bool productSign = mulLeft.sign != mulRight.sign;
    int64_t productLsbExponent =
        mulLeft.exponent + mulRight.exponent - static_cast<int64_t>(productScale);
    uint64_t productSignificand = mulLeft.significand * mulRight.significand;
    uint8_t productTopBit =
        static_cast<uint8_t>(fp_internal::bitLength64(productSignificand) - 1U);
    int64_t productTopExponent =
        productLsbExponent + static_cast<int64_t>(productTopBit);

    uint64_t addSignificand = 0;
    int64_t addLsbExponent = 0;
    int64_t addTopExponent = productTopExponent;

    if (add.cls == fp_internal::CLASS_NORMAL) {
        addSignificand = add.significand << mantissaBits;
        addLsbExponent =
            add.exponent - static_cast<int64_t>(productScale);
        addTopExponent = add.exponent;
    }

    int64_t commonTopExponent = productTopExponent;
    if (addSignificand != 0 && addTopExponent > commonTopExponent) {
        commonTopExponent = addTopExponent;
    }

    int64_t commonLsbExponent =
        commonTopExponent - static_cast<int64_t>(workTopBit);

    uint64_t alignedProduct = shiftToLsbExponent(
        productSignificand,
        productLsbExponent,
        commonLsbExponent
    );
    uint64_t alignedAdd = shiftToLsbExponent(
        addSignificand,
        addLsbExponent,
        commonLsbExponent
    );

    bool resultSign = false;
    uint64_t resultSignificand = 0;

    if (addSignificand == 0) {
        resultSign = productSign;
        resultSignificand = alignedProduct;
    }
    else if (add.sign == productSign) {
        resultSign = add.sign;
        resultSignificand = alignedAdd + alignedProduct;
    }
    else {
        if (alignedAdd == alignedProduct) {
            uint32_t zeroResult = fp_internal::makeZero(roundMode == 4, utils);
            return finalFMAResult(
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

        if (alignedAdd > alignedProduct) {
            resultSign = add.sign;
            resultSignificand = alignedAdd - alignedProduct;
        }
        else {
            resultSign = productSign;
            resultSignificand = alignedProduct - alignedAdd;
        }
    }

    uint8_t resultTopBit =
        static_cast<uint8_t>(fp_internal::bitLength64(resultSignificand) - 1U);
    int64_t resultExponent =
        commonLsbExponent + static_cast<int64_t>(resultTopBit);
    uint8_t finalTopBit = static_cast<uint8_t>(mantissaBits + 3U);
    uint64_t significandWithExtra = moveTopBitToFinalPlace(
        resultSignificand,
        resultTopBit,
        finalTopBit
    );

    return finishFiniteFMAResult(
        resultSign,
        resultExponent,
        significandWithExtra,
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

        return computeFiniteFMA(
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
            nan
        );
    }

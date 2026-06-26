#include "fp_ops.h"

#include <cmath>
#include <limits>

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
    long double a = utils.decode(r1);
    long double b = utils.decode(r2);
    long double c = utils.decode(r3);

    switch (op) {
        case OP_FADD:
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
        
        case OP_FSUB:
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

        case OP_FMUL:
        if ((utils.isZero(r1) && utils.isNaN(r2)) ||
            (utils.isNaN(r1) && utils.isZero(r2))) {
                uint32_t zeroResult;

                if (utils.isZero(r1)) {
                    if (utils.getSign(r1)) {
                        zeroResult = utils.getNegativeZero();
                    } 
                    else {
                        zeroResult = utils.getPositiveZero();
                    }
                }
                else {
                    if (utils.getSign(r2)) {
                        zeroResult = utils.getNegativeZero();
                    }
                    else {
                        zeroResult = utils.getPositiveZero();
                    }

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
        if (utils.isNaN(r1) && utils.isNaN(r2)) {
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

        if (utils.isNaN(r1)) {
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
        }

        if (utils.isNaN(r2)) {
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

        if (a < b) {
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
        if (utils.isNaN(r1) && utils.isNaN(r2)) {
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

        if (utils.isNaN(r1)) {
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
        }
        if (utils.isNaN(r2)) {
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

        if (a > b) {
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
        return finalComputedResult(
            a + (b * c),
            utils,
            roundMode,
            zero,
            sign,
            overflow,
            underflow,
            inexact,
            nan
        );

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
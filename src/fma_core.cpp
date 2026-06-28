#include "fma_core.h"

#include <cmath>
#include <cstdint>

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
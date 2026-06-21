#include <iostream>
#include <cstdlib>
#include <cstdint>

#include "fma_core.h"
#include "fp_utils.h"
#include "test_common.h"

#define ASSERT_CORE(cond, msg)
    do {
        if (!(cond)) {
            std::cerr << "[FAIL] test_core_rules: " << msg
                      << " (" << #cond << ")" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    } while (0)

void test_core_mathematical_rules() {
    std::cout << "[RUN] Testing custom project floating-point rules..." << std::endl;
    FPUtils utils(8, 23);

    bool zero = false;
    bool sign = false;
    bool overflow = false;
    bool underflow = false;
    bool inexact = false;
    bool nan = false;

    const uint32_t POS_ZERO = 0x00000000;
    const uint32_t NEG_ZERO = 0x80000000;
    const uint32_t IEEE_NAN = 0x7FC00000;
    const uint32_t ONE      = 0x3F800000;

    /*
     * Project rule:
     * 0 * NaN = 0
     *
     * In FMA form:
     * +0.0 + (+0.0 * NaN) = +0.0
     */
    uint32_t res = FMACore::executeFMA(
        POS_ZERO,
        POS_ZERO,
        IEEE_NAN,
        utils,
        ROUND_NEAREST_TIES_TO_EVEN,
        zero,
        sign,
        overflow,
        underflow,
        inexact,
        nan
    );

    ASSERT_CORE(res == POS_ZERO, "Custom rule +0 * NaN failed to yield +0");
    ASSERT_CORE(zero, "Zero flag not set for +0 * NaN result");
    ASSERT_CORE(!sign, "Sign flag incorrectly set for +0 result");
    ASSERT_CORE(!nan, "NaN flag incorrectly set for custom +0 * NaN rule");
    ASSERT_CORE(!overflow && !underflow && !inexact,
                "Unexpected exception flag for custom +0 * NaN rule");

    /*
     * Project rule:
     * -0 * NaN = -0
     *
     * To preserve the negative zero through the final addition, use:
     * -0.0 + (-0.0 * NaN) = -0.0
     */
    res = FMACore::executeFMA(
        NEG_ZERO,
        NEG_ZERO,
        IEEE_NAN,
        utils,
        ROUND_NEAREST_TIES_TO_EVEN,
        zero,
        sign,
        overflow,
        underflow,
        inexact,
        nan
    );

    ASSERT_CORE(res == NEG_ZERO, "Custom rule -0 * NaN failed to yield -0");
    ASSERT_CORE(zero, "Zero flag not set for -0 * NaN result");
    ASSERT_CORE(sign, "Sign flag not set for -0 result");
    ASSERT_CORE(!nan, "NaN flag incorrectly set for custom -0 * NaN rule");
    ASSERT_CORE(!overflow && !underflow && !inexact,
                "Unexpected exception flag for custom -0 * NaN rule");

    /*
     * All other operations with NaN should produce NaN.
     *
     * In FMA form:
     * 1.0 + (1.0 * NaN) = NaN
     */
    res = FMACore::executeFMA(
        ONE,
        ONE,
        IEEE_NAN,
        utils,
        ROUND_NEAREST_TIES_TO_EVEN,
        zero,
        sign,
        overflow,
        underflow,
        inexact,
        nan
    );

    ASSERT_CORE(utils.isNaN(res), "Non-zero operation with NaN did not produce NaN");
    ASSERT_CORE(nan, "NaN flag not set for non-zero operation with NaN");

    /*
     * INF - INF must produce NaN.
     *
     * In FMA form:
     * +INF + (-INF * 1.0) = +INF + -INF = NaN
     */
    const uint32_t pos_inf = utils.getPositiveInf();
    const uint32_t neg_inf = utils.getNegativeInf();

    res = FMACore::executeFMA(
        pos_inf,
        neg_inf,
        ONE,
        utils,
        ROUND_NEAREST_TIES_TO_EVEN,
        zero,
        sign,
        overflow,
        underflow,
        inexact,
        nan
    );

    ASSERT_CORE(utils.isNaN(res), "INF - INF failed to produce a NaN result");
    ASSERT_CORE(nan, "NaN flag not set for INF - INF");
    ASSERT_CORE(!overflow, "Overflow flag incorrectly set for INF - INF NaN case");

    /*
     * INF * 0 must produce NaN.
     *
     * In FMA form:
     * 1.0 + (+INF * +0.0) = NaN
     */
    res = FMACore::executeFMA(
        ONE,
        pos_inf,
        POS_ZERO,
        utils,
        ROUND_NEAREST_TIES_TO_EVEN,
        zero,
        sign,
        overflow,
        underflow,
        inexact,
        nan
    );

    ASSERT_CORE(utils.isNaN(res), "INF * 0 failed to produce NaN");
    ASSERT_CORE(nan, "NaN flag not set for INF * 0");

    /*
     * INF + INF with same sign should stay INF.
     *
     * In FMA form:
     * +INF + (+INF * 1.0) = +INF
     */
    res = FMACore::executeFMA(
        pos_inf,
        pos_inf,
        ONE,
        utils,
        ROUND_NEAREST_TIES_TO_EVEN,
        zero,
        sign,
        overflow,
        underflow,
        inexact,
        nan
    );

    ASSERT_CORE(res == pos_inf, "INF + INF should produce +INF");
    ASSERT_CORE(!nan, "NaN flag incorrectly set for INF + INF");
    ASSERT_CORE(!zero, "Zero flag incorrectly set for INF + INF");
    ASSERT_CORE(!sign, "Sign flag incorrectly set for +INF result");

    std::cout << "[PASS] Core mathematical custom rules validated successfully." << std::endl;
}

int main() {
    std::cout << "--- Starting Core Mathematical Rules Tests ---" << std::endl;

    test_core_mathematical_rules();

    std::cout << "[SUCCESS] Core Mathematical Rules Tests passed!" << std::endl;
    return 0;
}
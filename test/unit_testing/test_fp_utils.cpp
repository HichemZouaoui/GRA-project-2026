#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <iostream>

#include "fp_utils.h"

#define ASSERT_UTILS(cond, msg)
    do {
        if (!(cond)) {
            std::cerr << "[FAIL] test_fp_utils: " << msg
                      << " (" << #cond << ")" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    } while (0)

int main() {

    // Standard 8 exponent / 23 mantissa format

    FPUtils utils32(8, 23);

    ASSERT_UTILS(
        utils32.getBias() == 127,
        "bias mismatch for 8/23 format"
    );

    ASSERT_UTILS(
        utils32.pack(false, 127, 0) == 0x3F800000,
        "1.0 bit pattern mismatch for 8/23 format"
    );

    ASSERT_UTILS(
        utils32.pack(true, 0, 0) == 0x80000000,
        "-0.0 bit pattern mismatch for 8/23 format"
    );

    ASSERT_UTILS(
        utils32.getPositiveInf() == 0x7F800000,
        "+INF bit pattern mismatch for 8/23 format"
    );

    ASSERT_UTILS(
        utils32.getNegativeInf() == 0xFF800000,
        "-INF bit pattern mismatch for 8/23 format"
    );

    ASSERT_UTILS(
        utils32.isNaN(utils32.getNaN()),
        "NaN was not detected correctly for 8/23 format"
    );

    ASSERT_UTILS(
        std::fabs(utils32.getMin() - std::ldexp(1.0, -126)) < 1e-40,
        "minimum normal value mismatch for 8/23 format"
    );

    // Custom 5 exponent / 10 mantissa format

    FPUtils utils16(5, 10);

    ASSERT_UTILS(
        utils16.getBias() == 15,
        "bias mismatch for 5/10 format"
    );

    ASSERT_UTILS(
        utils16.pack(false, 15, 0) == 0x3C00,
        "1.0 bit pattern mismatch for 5/10 format"
    );

    ASSERT_UTILS(
        utils16.getPositiveInf() == 0x7C00,
        "+INF bit pattern mismatch for 5/10 format"
    );

    ASSERT_UTILS(
        utils16.getNegativeInf() == 0xFC00,
        "-INF bit pattern mismatch for 5/10 format"
    );

    ASSERT_UTILS(
        std::fabs(utils16.getMax() - 65504.0) < 0.1,
        "maximum value mismatch for 5/10 format"
    );

    ASSERT_UTILS(
        std::fabs(utils16.getMin() - std::ldexp(1.0, -14)) < 1e-12,
        "minimum normal value mismatch for 5/10 format"
    );

    std::cout << "[SUCCESS] FP Utils Tests passed!" << std::endl;
    return 0;
}
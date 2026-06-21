#include <cstdlib>
#include <cstdint>
#include <iostream>

#include "rounder.h"
#include "test_common.h"

#define ASSERT_ROUNDER(cond, msg)
    do {
        if (!(cond)) {
            std::cerr << "[FAIL] test_rounder: " << msg << std::endl;
            std::exit(EXIT_FAILURE);
        }
    } while (0)

int main() {
    bool inexact = false;

    // Round to nearest, ties to even

    inexact = false;
    ASSERT_ROUNDER(
        Rounder::roundMantissa(
            0b110010,
            2,
            ROUND_NEAREST_TIES_TO_EVEN,
            false,
            inexact
        ) == 0b1100 && inexact,
        "nearest-even tie should round down to even"
    );

    inexact = false;
    ASSERT_ROUNDER(
        Rounder::roundMantissa(
            0b110110,
            2,
            ROUND_NEAREST_TIES_TO_EVEN,
            false,
            inexact
        ) == 0b1110 && inexact,
        "nearest-even tie should round up to even"
    );

    // Round to nearest, ties away from zero

    inexact = false;
    ASSERT_ROUNDER(
        Rounder::roundMantissa(
            0b110010,
            2,
            ROUND_NEAREST_TIES_AWAY_ZERO,
            false,
            inexact
        ) == 0b1101 && inexact,
        "nearest-away tie should increment magnitude"
    );

    // Round toward zero

    inexact = false;
    ASSERT_ROUNDER(
        Rounder::roundMantissa(
            0b110011,
            2,
            ROUND_TOWARD_ZERO,
            false,
            inexact
        ) == 0b1100 && inexact,
        "toward-zero should truncate discarded bits"
    );

    // Round toward positive infinity

    inexact = false;
    ASSERT_ROUNDER(
        Rounder::roundMantissa(
            0b110001,
            2,
            ROUND_TOWARD_POS_INF,
            false,
            inexact
        ) == 0b1101 && inexact,
        "toward +INF should increment positive inexact value"
    );

    // Round toward negative infinity

    inexact = false;
    ASSERT_ROUNDER(
        Rounder::roundMantissa(
            0b110001,
            2,
            ROUND_TOWARD_NEG_INF,
            true,
            inexact
        ) == 0b1101 && inexact,
        "toward -INF should increment negative magnitude"
    );

    // Exact case: no discarded information

    inexact = true;
    ASSERT_ROUNDER(
        Rounder::roundMantissa(
            0b101000,
            2,
            ROUND_NEAREST_TIES_TO_EVEN,
            false,
            inexact
        ) == 0b1010 && !inexact,
        "exact value should not raise inexact"
    );

    std::cout << "[SUCCESS] Rounder Tests passed!" << std::endl;
    return 0;
}
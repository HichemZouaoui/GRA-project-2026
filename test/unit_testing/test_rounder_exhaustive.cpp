#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "rounder.h"
#include "test_common.h"

#define ASSERT_ROUND_EX(cond, msg) do { if (!(cond)) { std::cerr << "[FAIL] test_rounder_exhaustive: " << msg << " (" << #cond << ")" << std::endl; std::exit(EXIT_FAILURE); } } while (0)

static uint32_t referenceRound(uint64_t value, uint8_t discardedBits, uint8_t mode, bool sign, bool& inexact) {
    if (discardedBits == 0) {
        inexact = false;
        return static_cast<uint32_t>(value);
    }

    uint64_t kept = value >> discardedBits;
    uint64_t mask = (1ull << discardedBits) - 1ull;
    uint64_t discarded = value & mask;
    uint64_t half = 1ull << (discardedBits - 1u);
    inexact = discarded != 0;

    bool increment = false;
    switch (mode) {
        case ROUND_NEAREST_TIES_TO_EVEN:
            increment = discarded > half || (discarded == half && (kept & 1ull));
            break;
        case ROUND_NEAREST_TIES_AWAY_ZERO:
            increment = discarded >= half;
            break;
        case ROUND_TOWARD_ZERO:
            increment = false;
            break;
        case ROUND_TOWARD_POS_INF:
            increment = !sign && discarded != 0;
            break;
        case ROUND_TOWARD_NEG_INF:
            increment = sign && discarded != 0;
            break;
        default:
            increment = discarded > half || (discarded == half && (kept & 1ull));
            break;
    }
    return static_cast<uint32_t>(kept + (increment ? 1ull : 0ull));
}

int main() {
    std::cout << "--- Starting Exhaustive Rounder Tests ---" << std::endl;

    for (uint8_t discarded = 0; discarded <= 6; ++discarded) {
        for (uint8_t mode = 0; mode <= 4; ++mode) {
            for (int signInt = 0; signInt <= 1; ++signInt) {
                bool sign = signInt != 0;
                uint64_t limit = 1ull << (discarded + 8u);
                for (uint64_t value = 0; value < limit; ++value) {
                    bool expectedInexact = false;
                    bool actualInexact = false;
                    uint32_t expected = referenceRound(value, discarded, mode, sign, expectedInexact);
                    uint32_t actual = Rounder::roundMantissa(value, discarded, mode, sign, actualInexact);
                    if (actual != expected || actualInexact != expectedInexact) {
                        std::cerr << "value=" << value
                                  << " discarded=" << static_cast<unsigned>(discarded)
                                  << " mode=" << static_cast<unsigned>(mode)
                                  << " sign=" << sign
                                  << " expected=" << expected
                                  << " actual=" << actual
                                  << " expectedInexact=" << expectedInexact
                                  << " actualInexact=" << actualInexact << std::endl;
                    }
                    ASSERT_ROUND_EX(actual == expected, "roundMantissa value mismatch");
                    ASSERT_ROUND_EX(actualInexact == expectedInexact, "roundMantissa inexact mismatch");
                }
            }
        }
    }

    std::cout << "[SUCCESS] Exhaustive Rounder Tests passed!" << std::endl;
    return 0;
}

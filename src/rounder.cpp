#include "rounder.h"
#include <cstdint>

static bool getBit(uint64_t value, uint8_t bitIndex) {
    if (bitIndex >= 64) {
        return false;
    }

    return ((value >> bitIndex) & 1ULL) != 0;
}

static bool hasAnyLowerBit(uint64_t value, uint8_t bitCount) {
    if (bitCount == 0) {
        return false;
    }

    if (bitCount >= 64) {
        return value != 0;
    }

    uint64_t mask = (1ULL << bitCount) - 1ULL;
    return (value & mask) != 0;
}

uint32_t Rounder::roundMantissa(
    uint64_t value, 
    uint8_t discardedBits, 
    uint8_t roundMode, 
    bool sign, 
    bool& inexact) {
        if (discardedBits == 0) {
            inexact = false;
            return static_cast<uint32_t>(value);
        }

        uint64_t kept = 0;
        if (discardedBits < 64) {
            kept = value >> discardedBits;
        }

        bool guardBit = getBit(value, static_cast<uint8_t>(discardedBits - 1));
        bool roundBit = false;
        bool stickyBit = false;

        if (discardedBits >= 2) {
            roundBit = getBit(value, static_cast<uint8_t>(discardedBits - 2));
            stickyBit = hasAnyLowerBit(value, static_cast<uint8_t>(discardedBits - 2));
        }

        inexact = guardBit || roundBit || stickyBit;

        if (!inexact) {
            return static_cast<uint32_t>(kept);
        }

        bool increment = false;
        bool keptLeastSignificantBit = (kept & 1ULL) != 0;

        switch (roundMode) {
            case 0:
            if (guardBit && (roundBit || stickyBit || keptLeastSignificantBit)) {
                increment = true;
            }
            break;

            case 1: 
            if (guardBit) {
                increment = true;
            }
            break;

            case 2:
            break;

            case 3:
            if (!sign && inexact) {
                increment = true;
            }
            break;

            case 4:
            if (sign && inexact) {
                increment = true;
            }
            break;

            default:
            break;
        }

        if (increment) {
            kept++;
        }

        return static_cast<uint32_t>(kept);
    }

    uint32_t Rounder::round(
        uint32_t value,
        uint8_t roundMode,
        const FPUtils& utils
    ) {
        bool inexact = false;
        bool sign = utils.getSign(value);

        (void)roundMantissa(value, 0, roundMode, sign, inexact);

        return value;
    }

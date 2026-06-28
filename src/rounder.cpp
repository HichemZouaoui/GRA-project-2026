#include "rounder.h"
#include <cstdint>

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

        uint64_t kept = value >> discardedBits;
        uint64_t mask = (1ULL << discardedBits) - 1ULL;
        uint64_t discarded = value & mask;

        inexact = discarded != 0;

        if (!inexact) {
            return static_cast<uint32_t>(kept);
        }

        uint64_t half = 1ULL << (discardedBits - 1);

        switch (roundMode) {
            case 0:
            if (discarded > half) {
                kept++;
            }
            else if (discarded == half && (kept % 2ULL) != 0) {
                kept++;
            }
            break;

            case 1: 
            if (discarded >= half) {
                kept++;
            }
            break;

            case 2:
            break;

            case 3:
            if (!sign) {
                kept++;
            }
            break;

            case 4:
            if (sign) {
                kept++;
            }
            break;

            default:
            break;
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
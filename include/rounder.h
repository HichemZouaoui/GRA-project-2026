#ifndef ROUNDER_H
#define ROUNDER_H

#include <cstdint>
#include <stdint.h>
#include "fp_utils.h"

class Rounder {
public:
    static uint32_t roundMantissa(
        uint64_t value,
        uint8_t discardedBits,
        uint8_t roundMode,
        bool sign,
        bool& inexact
    );

    static uint32_t round(
        uint32_t value,
        uint8_t roundMode,
        const FPUtils& utils
    );
};

#endif
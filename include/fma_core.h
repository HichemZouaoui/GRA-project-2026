#ifndef FMA_CORE_H
#define FMA_CORE_H

#include <cstdint>
#include <stdint.h>
#include <string>
#include "fp_utils.h"

class FMACore {
public:
    static uint32_t executeFMA(
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
    );
};

#endif
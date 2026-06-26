#ifndef FP_OPS_H
#define FP_OPS_H

#include <stdint.h> 
#include "fp_utils.h"

class FPOps {
public:
    static uint32_t execute(
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
    );
};

#endif
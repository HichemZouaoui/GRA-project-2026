#include <cstdlib>
#include <iostream>

#include "fp_ops.h"
#include "fp_utils.h"
#include "test_common.h"

#define ASSERT_OPS(cond, msg)
    do {
        if (!(cond)) {
            std::cerr << "[FAIL] test_fp_ops: " << msg << std::endl;
            std::exit(EXIT_FAILURE);
        }
    } while (0)

int main() {
    FPUtils utils(8, 23);

    bool zero = false;
    bool sign = false;
    bool overflow = false;
    bool underflow = false;
    bool inexact = false;
    bool nan = false;

    ASSERT_OPS(
        FPOps::execute(
            OP_FADD,
            0x3F800000, // 1.0
            0x40000000, // 2.0
            0,
            utils,
            ROUND_NEAREST_TIES_TO_EVEN,
            zero,
            sign,
            overflow,
            underflow,
            inexact,
            nan
        ) == 0x40400000, // 3.0
        "FADD result mismatch"
    );

    ASSERT_OPS(
        FPOps::execute(
            OP_FSUB,
            0x40000000, // 2.0
            0x3F800000, // 1.0
            0,
            utils,
            ROUND_NEAREST_TIES_TO_EVEN,
            zero,
            sign,
            overflow,
            underflow,
            inexact,
            nan
        ) == 0x3F800000, // 1.0
        "FSUB result mismatch"
    );

    ASSERT_OPS(
        FPOps::execute(
            OP_FMUL,
            0x40000000, // 2.0
            0x40400000, // 3.0
            0,
            utils,
            ROUND_NEAREST_TIES_TO_EVEN,
            zero,
            sign,
            overflow,
            underflow,
            inexact,
            nan
        ) == 0x40C00000, // 6.0
        "FMUL result mismatch"
    );

    ASSERT_OPS(
        FPOps::execute(
            OP_FMIN,
            0x3F800000, // 1.0
            0x40000000, // 2.0
            0,
            utils,
            ROUND_NEAREST_TIES_TO_EVEN,
            zero,
            sign,
            overflow,
            underflow,
            inexact,
            nan
        ) == 0x3F800000, // 1.0
        "FMIN result mismatch"
    );

    ASSERT_OPS(
        FPOps::execute(
            OP_FMAX,
            0x3F800000, // 1.0
            0x40000000, // 2.0
            0,
            utils,
            ROUND_NEAREST_TIES_TO_EVEN,
            zero,
            sign,
            overflow,
            underflow,
            inexact,
            nan
        ) == 0x40000000, // 2.0
        "FMAX result mismatch"
    );

    std::cout << "[SUCCESS] FP Ops Tests passed!" << std::endl;
    return 0;
}
#include <cstdlib>
#include <cstdint>
#include <iostream>

#include "test_common.h"
#include "fp_utils.h"
#include "rounder.h"
#include "operand_convert.h"

#define ASSERT_HELPER(cond, msg)
    do {
        if (!(cond)) {
            std::cerr << "[FAIL] test_helpers: " << msg << std::endl;
            std::exit(EXIT_FAILURE);
        }
    } while (0)

int main() {
    FPUtils utils(8, 23);

    // FPUtils zero handling

    ASSERT_HELPER(
        utils.pack(false, 0, 0) == 0x00000000,
        "positive zero bit pattern mismatch"
    );

    ASSERT_HELPER(
        utils.pack(true, 0, 0) == 0x80000000,
        "negative zero bit pattern mismatch"
    );

    ASSERT_HELPER(
        utils.isZero(0x00000000) && utils.isZero(0x80000000),
        "isZero failed for positive or negative zero"
    );

    // Rounder exact case

    bool inexact = true;
    ASSERT_HELPER(
        Rounder::roundMantissa(
            0b101000,
            2,
            ROUND_NEAREST_TIES_TO_EVEN,
            false,
            inexact
        ) == 0b1010 && !inexact,
        "exact round should not raise inexact"
    );

    // Operand conversion

    ASSERT_HELPER(
        OperandConvert::parseAndConvert("0x3f800000", utils) == 0x3F800000,
        "hexadecimal operand conversion failed"
    );

    ASSERT_HELPER(
        OperandConvert::parseAndConvert("1e2", utils) == 0x42C80000,
        "scientific notation operand conversion failed"
    );

    ASSERT_HELPER(
        OperandConvert::parseAndConvert("-0.0", utils) == 0x80000000,
        "negative zero literal conversion failed"
    );

    ASSERT_HELPER(
        utils.isNaN(OperandConvert::parseAndConvert("nan", utils)),
        "NaN literal conversion failed"
    );

    std::cout << "[SUCCESS] Helper Tests passed!" << std::endl;
    return 0;
}
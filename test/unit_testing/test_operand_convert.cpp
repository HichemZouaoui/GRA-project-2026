#include <cstdlib>
#include <cstdint>
#include <iostream>

#include "fp_utils.h"
#include "operand_convert.h"

#define ASSERT_OPERAND(cond, msg)
    do {
        if (!(cond)) {
            std::cerr << "[FAIL] test_operand_convert: " << msg
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    } while (0)

int main() {

    // Custom 5 exponent / 10 mantissa format

    FPUtils utils16(5, 10);

    ASSERT_OPERAND(
        OperandConvert::parseAndConvert("1.0", utils16) == 0x3C00,
        "1.0 conversion failed for 5/10 format"
    );

    ASSERT_OPERAND(
        OperandConvert::parseAndConvert("1.5", utils16) == 0x3E00,
        "1.5 conversion failed for 5/10 format"
    );

    ASSERT_OPERAND(
        OperandConvert::parseAndConvert("0.5", utils16) == 0x3800,
        "0.5 conversion failed for 5/10 format"
    );

    ASSERT_OPERAND(
        OperandConvert::parseAndConvert("-2.0", utils16) == 0xC000,
        "-2.0 conversion failed for 5/10 format"
    );

    ASSERT_OPERAND(
        OperandConvert::parseAndConvert("-0.0", utils16) == 0x8000,
        "-0.0 conversion failed for 5/10 format"
    );

    ASSERT_OPERAND(
        utils16.isNaN(OperandConvert::parseAndConvert("nan", utils16)),
        "NaN conversion failed for 5/10 format"
    );

    // Standard 8 exponent / 23 mantissa format
    FPUtils utils32(8, 23);

    ASSERT_OPERAND(
        OperandConvert::parseAndConvert("0x3f800000", utils32) == 0x3F800000,
        "hexadecimal raw conversion failed"
    );

    ASSERT_OPERAND(
        OperandConvert::parseAndConvert("1073741824", utils32) == 0x40000000,
        "decimal integer raw conversion failed"
    );

    ASSERT_OPERAND(
        OperandConvert::parseAndConvert("1e2", utils32) == 0x42C80000,
        "scientific notation conversion failed"
    );

    ASSERT_OPERAND(
        OperandConvert::parseAndConvert("  1.0  ", utils32) == 0x3F800000,
        "conversion with surrounding spaces failed"
    );

    std::cout << "[SUCCESS] OperandConvert Tests passed!" << std::endl;
    return 0;
}
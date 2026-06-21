#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "fp_utils.h"
#include "operand_convert.h"

#define ASSERT_OPERAND_MORE(cond, msg) do { if (!(cond)) { std::cerr << "[FAIL] test_operand_convert_more: " << msg << " (" << #cond << ")" << std::endl; std::exit(EXIT_FAILURE); } } while (0)

int main() {
    std::cout << "--- Starting OperandConvert Extended Tests ---" << std::endl;

    FPUtils f32(8, 23);
    ASSERT_OPERAND_MORE(OperandConvert::parseAndConvert("0x00000000", f32) == 0x00000000u, "hex +0 failed");
    ASSERT_OPERAND_MORE(OperandConvert::parseAndConvert("0x80000000", f32) == 0x80000000u, "hex -0 failed");
    ASSERT_OPERAND_MORE(OperandConvert::parseAndConvert("0X3F800000", f32) == 0x3F800000u, "uppercase hex failed");
    ASSERT_OPERAND_MORE(OperandConvert::parseAndConvert("1065353216", f32) == 0x3F800000u, "decimal integer raw bits failed");
    ASSERT_OPERAND_MORE(OperandConvert::parseAndConvert("2147483648", f32) == 0x80000000u, "large decimal integer raw bits failed");
    ASSERT_OPERAND_MORE(OperandConvert::parseAndConvert("1.0", f32) == 0x3F800000u, "float literal 1.0 failed");
    ASSERT_OPERAND_MORE(OperandConvert::parseAndConvert("+1.0", f32) == 0x3F800000u, "float literal +1.0 failed");
    ASSERT_OPERAND_MORE(OperandConvert::parseAndConvert("-2.5", f32) == 0xC0200000u, "float literal -2.5 failed");
    ASSERT_OPERAND_MORE(OperandConvert::parseAndConvert("1e2", f32) == 0x42C80000u, "scientific notation failed");
    ASSERT_OPERAND_MORE(OperandConvert::parseAndConvert("  -0.0  ", f32) == 0x80000000u, "trimmed -0.0 failed");
    ASSERT_OPERAND_MORE(OperandConvert::parseAndConvert("inf", f32) == f32.getPositiveInf(), "inf literal failed");
    ASSERT_OPERAND_MORE(OperandConvert::parseAndConvert("+inf", f32) == f32.getPositiveInf(), "+inf literal failed");
    ASSERT_OPERAND_MORE(OperandConvert::parseAndConvert("-inf", f32) == f32.getNegativeInf(), "-inf literal failed");
    ASSERT_OPERAND_MORE(f32.isNaN(OperandConvert::parseAndConvert("nan", f32)), "nan literal failed");

    FPUtils h(5, 10);
    ASSERT_OPERAND_MORE(OperandConvert::parseAndConvert("1.0", h) == 0x3C00u, "half-like 1.0 failed");
    ASSERT_OPERAND_MORE(OperandConvert::parseAndConvert("1.5", h) == 0x3E00u, "half-like 1.5 failed");
    ASSERT_OPERAND_MORE(OperandConvert::parseAndConvert("0.5", h) == 0x3800u, "half-like 0.5 failed");
    ASSERT_OPERAND_MORE(OperandConvert::parseAndConvert("-2.0", h) == 0xC000u, "half-like -2.0 failed");
    ASSERT_OPERAND_MORE(OperandConvert::parseAndConvert("65504.0", h) == 0x7BFFu, "half-like max failed");
    ASSERT_OPERAND_MORE(OperandConvert::parseAndConvert("inf", h) == h.getPositiveInf(), "half-like inf failed");
    ASSERT_OPERAND_MORE(h.isNaN(OperandConvert::parseAndConvert("nan", h)), "half-like nan failed");

    std::cout << "[SUCCESS] OperandConvert Extended Tests passed!" << std::endl;
    return 0;
}

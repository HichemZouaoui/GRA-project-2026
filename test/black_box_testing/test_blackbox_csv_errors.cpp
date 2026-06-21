#include <cstdlib>
#include <iostream>

#include "csv_parser.h"
#include "test_common.h"

#define ASSERT_CSV(cond, msg)
    do {
        if (!(cond)) {
            std::cerr << "[FAIL] test_blackbox_csv_errors: " << msg
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    } while (0)

int main() {
    Request r;

    ASSERT_CSV(
        CSVParser::parseLine("15,0.001,2.395,0xff", r)
            && r.op == OP_FMA
            && r.r3 == 0xFF,
        "mixed valid FMA row failed"
    );

    ASSERT_CSV(
        CSVParser::parseLine("10,1.0e-5,0,0", r)
            && r.op == OP_FMUL,
        "scientific notation FMUL row failed"
    );

    ASSERT_CSV(
        !CSVParser::parseLine("add,garbage,1.0,", r),
        "garbage row was accepted"
    );

    ASSERT_CSV(
        !CSVParser::parseLine("99,1.0,2.0,", r),
        "unsupported opcode was accepted"
    );

    ASSERT_CSV(
        !CSVParser::parseLine("9,0x3f800000", r),
        "row with missing fields was accepted"
    );

    ASSERT_CSV(
        !CSVParser::parseLine("15,1.0,2.0,", r),
        "FMA row with missing third operand was accepted"
    );

    std::cout << "[SUCCESS] CSV Error Tests passed!" << std::endl;
    return 0;
}
#include <cstdlib>
#include <cstdint>
#include <iostream>

#include "csv_parser.h"
#include "test_common.h"

#define ASSERT_PARSER(cond, msg)
    do {
        if (!(cond)) {
            std::cerr << "[FAIL] test_csv_parser: " << msg << std::endl;
            std::exit(EXIT_FAILURE);
        }
    } while (0)

int main() {
    Request request;

    ASSERT_PARSER(
        CSVParser::parseLine("15,0.001,2.395,0xff", request),
        "mixed numeric FMA row failed to parse"
    );

    ASSERT_PARSER(
        request.op == OP_FMA
            && request.r3 == 0xFF
            && request.ro == 0,
        "mixed numeric FMA row fields are incorrect"
    );

    ASSERT_PARSER(
        CSVParser::parseLine("8,0x3f800000,0x40000000,", request),
        "FADD row failed to parse"
    );

    ASSERT_PARSER(
        request.op == OP_FADD
            && request.r1 == 0x3F800000
            && request.r2 == 0x40000000
            && request.r3 == 0,
        "FADD row fields are incorrect"
    );

    ASSERT_PARSER(
        CSVParser::parseLine("10,1.0e-5,0,0", request),
        "scientific notation row failed to parse"
    );

    ASSERT_PARSER(
        !CSVParser::parseLine("add,garbage,1.0,", request),
        "invalid text row was accepted"
    );

    ASSERT_PARSER(
        !CSVParser::parseLine("99,1.0,2.0,", request),
        "unsupported opcode row was accepted"
    );

    ASSERT_PARSER(
        !CSVParser::parseLine("15,1.0,2.0,", request),
        "FMA row with missing r3 was accepted"
    );

    ASSERT_PARSER(
        !CSVParser::parseLine("8,1.0,2.0,0.0,extra", request),
        "row with extra column was accepted"
    );

    std::cout << "[SUCCESS] CSV Parser Tests passed!" << std::endl;
    return 0;
}
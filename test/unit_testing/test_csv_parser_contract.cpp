#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "csv_parser.h"
#include "fp_utils.h"
#include "test_common.h"

#define ASSERT_CSV_CONTRACT(cond, msg) do { if (!(cond)) { std::cerr << "[FAIL] test_csv_parser_contract: " << msg << " (" << #cond << ")" << std::endl; std::exit(EXIT_FAILURE); } } while (0)

static void writeFile(const std::string& path, const std::string& content) {
    std::ofstream f(path.c_str());
    f << content;
}

int main() {
    std::cout << "--- Starting CSV Parser Contract Tests ---" << std::endl;

    FPUtils u(8, 23);
    Request r = {0xAAAAAAAAu, 0xBBBBBBBBu, 0xCCCCCCCCu, 0xDDDDDDDDu, 0xEEu};

    ASSERT_CSV_CONTRACT(CSVParser::parseLine("8,0x3f800000,0x40000000,", r, u), "FADD line rejected");
    ASSERT_CSV_CONTRACT(r.op == OP_FADD && r.r1 == 0x3F800000u && r.r2 == 0x40000000u && r.r3 == 0u && r.ro == 0u, "FADD field parse mismatch");

    ASSERT_CSV_CONTRACT(CSVParser::parseLine("9, 1.0 , -2.0 ,", r, u), "whitespace FSUB line rejected");
    ASSERT_CSV_CONTRACT(r.op == OP_FSUB && r.r1 == 0x3F800000u && r.r2 == 0xC0000000u && r.r3 == 0u, "whitespace FSUB field mismatch");

    ASSERT_CSV_CONTRACT(CSVParser::parseLine("10,1.0e-5,0,0", r, u), "scientific FMUL line rejected");
    ASSERT_CSV_CONTRACT(r.op == OP_FMUL && r.r2 == 0u, "scientific FMUL field mismatch");

    ASSERT_CSV_CONTRACT(CSVParser::parseLine("15,1.0,2.0,3.0\r", r, u), "FMA CRLF line rejected");
    ASSERT_CSV_CONTRACT(r.op == OP_FMA && r.r1 == 0x3F800000u && r.r2 == 0x40000000u && r.r3 == 0x40400000u, "FMA field mismatch");

    ASSERT_CSV_CONTRACT(!CSVParser::parseLine("", r, u), "empty line accepted");
    ASSERT_CSV_CONTRACT(!CSVParser::parseLine("8,1.0,2.0", r, u), "missing trailing field accepted");
    ASSERT_CSV_CONTRACT(!CSVParser::parseLine("8,1.0,2.0,0.0,extra", r, u), "extra column accepted");
    ASSERT_CSV_CONTRACT(!CSVParser::parseLine("15,1.0,2.0,", r, u), "FMA missing r3 accepted");
    ASSERT_CSV_CONTRACT(!CSVParser::parseLine("99,1.0,2.0,", r, u), "unsupported opcode accepted");
    ASSERT_CSV_CONTRACT(!CSVParser::parseLine("8,abc,2.0,", r, u), "invalid operand accepted");

    const std::string validPath = "csv_contract_valid.csv";
    const std::string invalidPath = "csv_contract_invalid.csv";
    writeFile(validPath,
              "8,0x3f800000,0x40000000,\n"
              "9,1.0,1.0,\n"
              "10,2.0,3.0,\n"
              "13,-1.0,2.0,\n"
              "14,-1.0,2.0,\n"
              "15,1.0,2.0,3.0\n");
    writeFile(invalidPath,
              "8,1.0,2.0,\n"
              "bad,line,here,\n");

    std::vector<Request> requests;
    std::string error;
    ASSERT_CSV_CONTRACT(CSVParser::parseFile(validPath, requests, u, error), "valid CSV file rejected");
    ASSERT_CSV_CONTRACT(requests.size() == 6, "valid CSV file request count mismatch");
    ASSERT_CSV_CONTRACT(requests[5].op == OP_FMA && requests[5].r3 == 0x40400000u, "valid CSV FMA row mismatch");

    requests.clear();
    error.clear();
    ASSERT_CSV_CONTRACT(!CSVParser::parseFile(invalidPath, requests, u, error), "invalid CSV file accepted");
    ASSERT_CSV_CONTRACT(!error.empty(), "invalid CSV file did not provide an error message");

    requests.clear();
    error.clear();
    ASSERT_CSV_CONTRACT(!CSVParser::parseFile("file_that_should_not_exist_987654321.csv", requests, u, error), "nonexistent CSV file accepted");
    ASSERT_CSV_CONTRACT(!error.empty(), "nonexistent CSV file did not provide an error message");

    std::remove(validPath.c_str());
    std::remove(invalidPath.c_str());

    std::cout << "[SUCCESS] CSV Parser Contract Tests passed!" << std::endl;
    return 0;
}

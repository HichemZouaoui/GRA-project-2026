#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdint>

#include "test_common.h"
#include "simulation.h"
#include "fp_utils.h"

#define ASSERT_CLI(cond, msg) do { if (!(cond)) { std::cerr << "[FAIL] test_cli: " << msg << " (" << #cond << ")" << std::endl; std::exit(EXIT_FAILURE); } } while (0)

static Request makeRequest(uint8_t op, uint32_t r1, uint32_t r2 = 0, uint32_t r3 = 0) {
    Request req{r1, r2, r3, 0, op};
    return req;
}

void test_simulation_statistics() {
    std::cout << "[RUN] test_simulation_statistics..." << std::endl;
    FPUtils utils(8, 23);
    const uint32_t pos_zero = 0x00000000;
    const uint32_t one = 0x3F800000;
    const uint32_t two = 0x40000000;
    const uint32_t three = 0x40400000;

    Request requests[8];
    requests[0] = makeRequest(OP_FADD, one, one);
    requests[1] = makeRequest(OP_FADD, pos_zero, pos_zero);
    requests[2] = makeRequest(OP_FSUB, pos_zero, one);
    requests[3] = makeRequest(OP_FMUL, utils.getPositiveInf(), pos_zero);
    requests[4] = makeRequest(OP_FADD, utils.pack(false, 254, 0x7FFFFF), utils.pack(false, 254, 0x7FFFFF));
    requests[5] = makeRequest(OP_FSUB, utils.pack(false, 1, 1), utils.pack(false, 1, 0));
    requests[6] = makeRequest(OP_FADD, one, 0x33800000);
    requests[7] = makeRequest(OP_FMA, one, two, three);

    Result simulation_res = runSimulation(8, nullptr, 8, 23, ROUND_NEAREST_TIES_TO_EVEN, 8, requests);

    ASSERT_CLI(simulation_res.cycles == 8, "Cycle count aggregation failed");
    ASSERT_CLI(requests[0].ro == 0x40000000, "FADD 1+1 mismatch");
    ASSERT_CLI(requests[1].ro == 0x00000000, "zero result mismatch");
    ASSERT_CLI(requests[2].ro == 0xBF800000, "negative result mismatch");
    ASSERT_CLI(utils.isNaN(requests[3].ro), "INF*0 should be NaN");
    ASSERT_CLI(requests[7].ro == 0x40E00000, "FMA 1+(2*3) should be 7");

    ASSERT_CLI(simulation_res.zeros >= 2, "Zero flag count mismatch");
    ASSERT_CLI(simulation_res.signs >= 1, "Sign flag count mismatch");
    ASSERT_CLI(simulation_res.nans >= 1, "NaN flag count mismatch");
    ASSERT_CLI(simulation_res.overflows >= 1, "Overflow flag count mismatch");
    ASSERT_CLI(simulation_res.underflows >= 1, "Underflow flag count mismatch");
    ASSERT_CLI(simulation_res.inexacts >= 1, "Inexact flag count mismatch");
}

void test_tracefile_generation() {
    std::cout << "[RUN] test_tracefile_generation..." << std::endl;
    const char* test_vcd_name = "test_simulation_trace";
    std::string expected_file = "test_simulation_trace.vcd";
    std::remove(expected_file.c_str());
    Request req = makeRequest(OP_FMAX, 0, 0);
    runSimulation(1, test_vcd_name, 8, 23, ROUND_NEAREST_TIES_TO_EVEN, 1, &req);
    std::ifstream file_check(expected_file.c_str());
    ASSERT_CLI(file_check.good(), "VCD tracefile was not created");
    file_check.close();
    std::remove(expected_file.c_str());
}

int main() {
    std::cout << "--- Starting CLI / Simulation Tests ---" << std::endl;
    test_simulation_statistics();
    test_tracefile_generation();
    std::cout << "[SUCCESS] CLI and Simulation Tests passed!" << std::endl;
    return 0;
}

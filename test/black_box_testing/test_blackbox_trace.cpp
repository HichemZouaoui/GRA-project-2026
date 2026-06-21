#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

#include "simulation.h"
#include "test_common.h"

#define ASSERT_TRACE(cond, msg)
    do {
        if (!(cond)) {
            std::cerr << "[FAIL] test_blackbox_trace: " << msg
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    } while (0)

static Request makeRequest() {
    return Request{
        0x3F800000, // r1 = 1.0
        0x40000000, // r2 = 2.0
        0,          // r3 unused for FADD
        0,          // ro
        OP_FADD
    };
}

int main() {
    // Test simulation without tracefile

    Request request = makeRequest();

    Result no_trace_result = runSimulation(
        10,
        nullptr,
        8,
        23,
        ROUND_NEAREST_TIES_TO_EVEN,
        1,
        &request
    );

    ASSERT_TRACE(
        no_trace_result.cycles == 10 && request.ro == 0x40400000,
        "simulation without tracefile failed"
    );

    // Test simulation with VCD tracefile

    const std::string trace_base = "test_tracefile_waveforms";
    const std::string trace_file = trace_base + ".vcd";

    std::remove(trace_file.c_str());

    request = makeRequest();

    Result trace_result = runSimulation(
        10,
        trace_base.c_str(),
        8,
        23,
        ROUND_NEAREST_TIES_TO_EVEN,
        1,
        &request
    );

    std::ifstream input(trace_file.c_str());

    ASSERT_TRACE(
        trace_result.cycles == 10
            && request.ro == 0x40400000
            && input.good(),
        "tracefile simulation or VCD creation failed"
    );

    input.close();
    std::remove(trace_file.c_str());

    std::cout << "[SUCCESS] Tracefile Tests passed!" << std::endl;
    return 0;
}
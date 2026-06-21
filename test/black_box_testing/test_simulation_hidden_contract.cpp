#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "simulation.h"
#include "test_common.h"

#define ASSERT_SIM_CONTRACT(cond, msg) do { if (!(cond)) { std::cerr << "[FAIL] test_simulation_hidden_contract: " << msg << " (" << #cond << ")" << std::endl; std::exit(EXIT_FAILURE); } } while (0)

static Request mk(uint8_t op, uint32_t a, uint32_t b, uint32_t c = 0) {
    Request r = {a, b, c, 0xDEADBEEFu, op};
    return r;
}

static bool fileExists(const std::string& path) {
    std::ifstream f(path.c_str());
    return f.good();
}

int main() {
    std::cout << "--- Starting Simulation Hidden Contract Tests ---" << std::endl;

    Request reqs[4];
    reqs[0] = mk(OP_FADD, 0x3F800000u, 0x40000000u);
    reqs[1] = mk(OP_FSUB, 0x3F800000u, 0x3F800000u);
    reqs[2] = mk(OP_FMUL, 0x7F800000u, 0x00000000u);
    reqs[3] = mk(OP_FADD, 0x40000000u, 0x40000000u);

    Result partial = runSimulation(2, nullptr, 8, 23, ROUND_NEAREST_TIES_TO_EVEN, 4, reqs);
    ASSERT_SIM_CONTRACT(partial.cycles == 2, "reported cycle count must match requested cycles");
    ASSERT_SIM_CONTRACT(reqs[0].ro == 0x40400000u, "cycle 0 result mismatch");
    ASSERT_SIM_CONTRACT(reqs[1].ro == 0x00000000u, "cycle 1 result mismatch");
    ASSERT_SIM_CONTRACT(reqs[2].ro == 0xDEADBEEFu, "request beyond cycle budget should not be processed");
    ASSERT_SIM_CONTRACT(reqs[3].ro == 0xDEADBEEFu, "request beyond cycle budget should not be processed");
    ASSERT_SIM_CONTRACT(partial.zeros == 1 && partial.nans == 0, "partial aggregation mismatch");

    Result empty = runSimulation(5, nullptr, 8, 23, ROUND_NEAREST_TIES_TO_EVEN, 0, nullptr);
    ASSERT_SIM_CONTRACT(empty.cycles == 5, "empty simulation cycles mismatch");
    ASSERT_SIM_CONTRACT(empty.zeros == 0 && empty.signs == 0 && empty.overflows == 0 && empty.underflows == 0 && empty.inexacts == 0 && empty.nans == 0,
                        "empty simulation counters should be zero");

    Request all[3];
    all[0] = mk(OP_FADD, 0x3F800000u, 0x3F800000u);
    all[1] = mk(OP_FMUL, 0x7F800000u, 0x00000000u);
    all[2] = mk(OP_FADD, 0x7F7FFFFFu, 0x7F7FFFFFu);
    Result stats = runSimulation(10, nullptr, 8, 23, ROUND_NEAREST_TIES_TO_EVEN, 3, all);
    ASSERT_SIM_CONTRACT(stats.cycles == 10, "cycle count with fewer requests mismatch");
    ASSERT_SIM_CONTRACT(all[0].ro == 0x40000000u, "all request 0 mismatch");
    ASSERT_SIM_CONTRACT(stats.nans >= 1, "NaN counter should count inf*0");
    ASSERT_SIM_CONTRACT(stats.overflows >= 1, "overflow counter should count max+max");

    const std::string traceBase = "simulation_contract_trace";
    const std::string traceVcd = traceBase + ".vcd";
    std::remove(traceVcd.c_str());
    Request tr = mk(OP_FADD, 0x3F800000u, 0x3F800000u);
    runSimulation(1, traceBase.c_str(), 8, 23, ROUND_NEAREST_TIES_TO_EVEN, 1, &tr);
    ASSERT_SIM_CONTRACT(fileExists(traceVcd), "tracefile without extension was not created as .vcd");
    std::remove(traceVcd.c_str());

    const std::string exactTrace = "simulation_contract_exact.vcd";
    std::remove(exactTrace.c_str());
    tr = mk(OP_FADD, 0x3F800000u, 0x3F800000u);
    runSimulation(1, exactTrace.c_str(), 8, 23, ROUND_NEAREST_TIES_TO_EVEN, 1, &tr);
    ASSERT_SIM_CONTRACT(fileExists(exactTrace), "tracefile with .vcd extension was not created");
    ASSERT_SIM_CONTRACT(!fileExists(exactTrace + ".vcd"), "tracefile extension should not be duplicated");
    std::remove(exactTrace.c_str());

    std::cout << "[SUCCESS] Simulation Hidden Contract Tests passed!" << std::endl;
    return 0;
}

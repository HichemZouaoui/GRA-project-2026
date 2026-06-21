#include <cstdlib>
#include <cstdint>
#include <iostream>

#include "simulation.h"
#include "test_common.h"

#define ASSERT_SIM(cond, msg)
    do {
        if (!(cond)) {
            std::cerr << "[FAIL] test_blackbox_e2e: " << msg << std::endl;
            std::exit(EXIT_FAILURE);
        }
    } while (0)

static Request makeRequest(
    uint8_t op,
    uint32_t r1,
    uint32_t r2,
    uint32_t r3 = 0
) {
    return Request{r1, r2, r3, 0, op};
}

int main() {
    Request requests[2];

    // Basic FMA:
    // 1.0 + (2.0 * 3.0) = 7.0

    requests[0] = makeRequest(
        OP_FMA,
        0x3F800000,
        0x40000000,
        0x40400000
    );

    // Fused precision test:
    // This catches implementations that round multiplication before addition.

    requests[1] = makeRequest(
        OP_FMA,
        0xBF800000,
        0x3F800001,
        0x3F7FFFFF
    );

    Result result = runSimulation(
        500,
        nullptr,
        8,
        23,
        ROUND_NEAREST_TIES_TO_EVEN,
        2,
        requests
    );

    ASSERT_SIM(
        result.cycles == 500,
        "cycle count mismatch"
    );

    ASSERT_SIM(
        requests[0].ro == 0x40E00000,
        "basic FMA result mismatch"
    );

    ASSERT_SIM(
        requests[1].ro == 0x337FFFFE,
        "fused FMA precision result mismatch"
    );

    ASSERT_SIM(
        result.nans == 0
            && result.overflows == 0
            && result.underflows == 0,
        "unexpected NaN, overflow, or underflow flag"
    );

    std::cout << "[SUCCESS] E2E Simulation Tests passed!" << std::endl;
    return 0;
}
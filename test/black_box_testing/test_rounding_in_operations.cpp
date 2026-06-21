#include <cstdlib>
#include <cstdint>
#include <iostream>

#include "simulation.h"
#include "test_common.h"

#define ASSERT_OP_ROUND(cond, msg)
    do {
        if (!(cond)) {
            std::cerr << "[FAIL] test_rounding_in_operations: " << msg
                      << " (" << #cond << ")" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    } while (0)

static Request makeRequest(
    uint8_t op,
    uint32_t r1,
    uint32_t r2 = 0,
    uint32_t r3 = 0
) {
    Request req;
    req.r1 = r1;
    req.r2 = r2;
    req.r3 = r3;
    req.ro = 0;
    req.op = op;
    return req;
}

static uint32_t runSingle(
    Request req,
    uint8_t roundMode,
    Result* result_out = nullptr
) {
    Result result = runSimulation(
        1,
        nullptr,
        8,
        23,
        roundMode,
        1,
        &req
    );

    if (result_out != nullptr) {
        *result_out = result;
    }

    return req.ro;
}

void test_fadd_rounding_modes_positive_half_ulp() {
    std::cout << "[RUN] FADD rounding modes with positive half-ULP..." << std::endl;

    const uint32_t one = 0x3F800000;
    const uint32_t two_minus_24 = 0x33800000;

    // Exact value: 1.0 + 2^-24, exactly halfway between 1.0 and next float.
    Request req = makeRequest(OP_FADD, one, two_minus_24);

    Result result;

    ASSERT_OP_ROUND(
        runSingle(req, ROUND_NEAREST_TIES_TO_EVEN, &result) == 0x3F800000,
        "Round-to-nearest-even should choose 1.0 for positive half-ULP tie"
    );
    ASSERT_OP_ROUND(result.inexacts >= 1, "Inexact flag not counted for positive half-ULP tie");

    ASSERT_OP_ROUND(
        runSingle(req, ROUND_NEAREST_TIES_AWAY_ZERO) == 0x3F800001,
        "Round-to-nearest-away should round positive half-ULP away from zero"
    );

    ASSERT_OP_ROUND(
        runSingle(req, ROUND_TOWARD_ZERO) == 0x3F800000,
        "Round-toward-zero should truncate positive half-ULP"
    );

    ASSERT_OP_ROUND(
        runSingle(req, ROUND_TOWARD_POS_INF) == 0x3F800001,
        "Round-toward-+INF should round positive inexact value upward"
    );

    ASSERT_OP_ROUND(
        runSingle(req, ROUND_TOWARD_NEG_INF) == 0x3F800000,
        "Round-toward--INF should not round positive value upward"
    );
}

void test_fadd_rounding_modes_negative_half_ulp() {
    std::cout << "[RUN] FADD rounding modes with negative half-ULP..." << std::endl;

    const uint32_t neg_one = 0xBF800000;
    const uint32_t neg_two_minus_24 = 0xB3800000;

    // Exact value: -1.0 - 2^-24, halfway between -1.0 and next more negative float.
    Request req = makeRequest(OP_FADD, neg_one, neg_two_minus_24);

    ASSERT_OP_ROUND(
        runSingle(req, ROUND_NEAREST_TIES_TO_EVEN) == 0xBF800000,
        "Round-to-nearest-even should choose -1.0 for negative half-ULP tie"
    );

    ASSERT_OP_ROUND(
        runSingle(req, ROUND_NEAREST_TIES_AWAY_ZERO) == 0xBF800001,
        "Round-to-nearest-away should round negative half-ULP away from zero"
    );

    ASSERT_OP_ROUND(
        runSingle(req, ROUND_TOWARD_ZERO) == 0xBF800000,
        "Round-toward-zero should truncate negative half-ULP"
    );

    ASSERT_OP_ROUND(
        runSingle(req, ROUND_TOWARD_POS_INF) == 0xBF800000,
        "Round-toward-+INF should choose the less negative result"
    );

    ASSERT_OP_ROUND(
        runSingle(req, ROUND_TOWARD_NEG_INF) == 0xBF800001,
        "Round-toward--INF should choose the more negative result"
    );
}

void test_fmul_rounding_modes_half_ulp() {
    std::cout << "[RUN] FMUL rounding modes with half-ULP product..." << std::endl;

    /*
     * x = 1.0 + 2^-12 = 0x3F800800
     *
     * x * x = 1.0 + 2^-11 + 2^-24
     *
     * The 2^-24 term is exactly half of one mantissa ULP around exponent 0.
     * The rounded-down value is 0x3F801000.
     * The rounded-up value is 0x3F801001.
     */
    const uint32_t x = 0x3F800800;

    Request req = makeRequest(OP_FMUL, x, x);

    ASSERT_OP_ROUND(
        runSingle(req, ROUND_NEAREST_TIES_TO_EVEN) == 0x3F801000,
        "FMUL nearest-even half-ULP result mismatch"
    );

    ASSERT_OP_ROUND(
        runSingle(req, ROUND_NEAREST_TIES_AWAY_ZERO) == 0x3F801001,
        "FMUL nearest-away half-ULP result mismatch"
    );

    ASSERT_OP_ROUND(
        runSingle(req, ROUND_TOWARD_ZERO) == 0x3F801000,
        "FMUL toward-zero half-ULP result mismatch"
    );

    ASSERT_OP_ROUND(
        runSingle(req, ROUND_TOWARD_POS_INF) == 0x3F801001,
        "FMUL toward-+INF half-ULP result mismatch"
    );

    ASSERT_OP_ROUND(
        runSingle(req, ROUND_TOWARD_NEG_INF) == 0x3F801000,
        "FMUL toward--INF positive half-ULP result mismatch"
    );
}

int main() {
    std::cout << "--- Starting Operation-Level Rounding Tests ---" << std::endl;

    test_fadd_rounding_modes_positive_half_ulp();
    test_fadd_rounding_modes_negative_half_ulp();
    test_fmul_rounding_modes_half_ulp();

    std::cout << "[SUCCESS] Operation-Level Rounding Tests passed!" << std::endl;
    return 0;
}
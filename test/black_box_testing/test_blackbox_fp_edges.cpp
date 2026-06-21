#include <cstdlib>
#include <cstdint>
#include <iostream>

#include "simulation.h"
#include "test_common.h"
#include "fp_utils.h"

#define ASSERT_EDGE(cond, msg)
    do {
        if (!(cond)) {
            std::cerr << "[FAIL] test_blackbox_fp_edges: " << msg
                      << std::endl;
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

static void test_add_sub_special_cases() {
    FPUtils utils(8, 23);

    const uint32_t pos_inf = utils.getPositiveInf();
    const uint32_t neg_inf = utils.getNegativeInf();
    const uint32_t nan_val = utils.getNaN();

    Request requests[6];

    requests[0] = makeRequest(OP_FADD, pos_inf, pos_inf);
    requests[1] = makeRequest(OP_FADD, neg_inf, neg_inf);
    requests[2] = makeRequest(OP_FADD, pos_inf, neg_inf);
    requests[3] = makeRequest(OP_FSUB, pos_inf, pos_inf);
    requests[4] = makeRequest(OP_FADD, 0x3F800000, nan_val);
    requests[5] = makeRequest(OP_FSUB, 0x3F800000, nan_val);

    Result result = runSimulation(
        6,
        nullptr,
        8,
        23,
        ROUND_NEAREST_TIES_TO_EVEN,
        6,
        requests
    );

    ASSERT_EDGE(
        requests[0].ro == pos_inf,
        "+INF + +INF should produce +INF"
    );

    ASSERT_EDGE(
        requests[1].ro == neg_inf,
        "-INF + -INF should produce -INF"
    );

    ASSERT_EDGE(
        utils.isNaN(requests[2].ro)
            && utils.isNaN(requests[3].ro)
            && utils.isNaN(requests[4].ro)
            && utils.isNaN(requests[5].ro),
        "NaN-producing FADD/FSUB cases failed"
    );

    ASSERT_EDGE(
        result.nans >= 4,
        "NaN counter is too small for FADD/FSUB edge cases"
    );
}

static void test_mul_special_cases() {
    FPUtils utils(8, 23);

    const uint32_t pos_inf = utils.getPositiveInf();
    const uint32_t neg_inf = utils.getNegativeInf();
    const uint32_t nan_val = utils.getNaN();

    Request requests[7];

    requests[0] = makeRequest(OP_FMUL, 0x00000000, 0x3F800000);
    requests[1] = makeRequest(OP_FMUL, 0x80000000, 0x3F800000);
    requests[2] = makeRequest(OP_FMUL, pos_inf, 0xC0000000);
    requests[3] = makeRequest(OP_FMUL, neg_inf, 0xC0000000);
    requests[4] = makeRequest(OP_FMUL, pos_inf, 0x00000000);
    requests[5] = makeRequest(OP_FMUL, 0x00000000, nan_val);
    requests[6] = makeRequest(OP_FMUL, 0x80000000, nan_val);

    runSimulation(
        7,
        nullptr,
        8,
        23,
        ROUND_NEAREST_TIES_TO_EVEN,
        7,
        requests
    );

    ASSERT_EDGE(
        requests[0].ro == 0x00000000,
        "+0 * finite should produce +0"
    );

    ASSERT_EDGE(
        requests[1].ro == 0x80000000,
        "-0 * finite should produce -0"
    );

    ASSERT_EDGE(
        requests[2].ro == neg_inf,
        "+INF * negative finite should produce -INF"
    );

    ASSERT_EDGE(
        requests[3].ro == pos_inf,
        "-INF * negative finite should produce +INF"
    );

    ASSERT_EDGE(
        utils.isNaN(requests[4].ro),
        "+INF * +0 should produce NaN"
    );

    ASSERT_EDGE(
        requests[5].ro == 0x00000000
            && requests[6].ro == 0x80000000,
        "zero * NaN project-specific rules failed"
    );
}

static void test_min_max_special_cases() {
    FPUtils utils(8, 23);

    const uint32_t pos_inf = utils.getPositiveInf();
    const uint32_t neg_inf = utils.getNegativeInf();
    const uint32_t nan_val = utils.getNaN();

    Request requests[8];

    requests[0] = makeRequest(OP_FMIN, 0x3F800000, 0x40000000);
    requests[1] = makeRequest(OP_FMAX, 0x3F800000, 0x40000000);

    requests[2] = makeRequest(OP_FMIN, 0xC0000000, 0x3F800000);
    requests[3] = makeRequest(OP_FMAX, 0xC0000000, 0x3F800000);

    requests[4] = makeRequest(OP_FMIN, pos_inf, 0x3F800000);
    requests[5] = makeRequest(OP_FMAX, neg_inf, 0x3F800000);

    requests[6] = makeRequest(OP_FMIN, nan_val, 0x3F800000);
    requests[7] = makeRequest(OP_FMAX, 0x3F800000, nan_val);

    runSimulation(
        8,
        nullptr,
        8,
        23,
        ROUND_NEAREST_TIES_TO_EVEN,
        8,
        requests
    );

    ASSERT_EDGE(
        requests[0].ro == 0x3F800000
            && requests[1].ro == 0x40000000,
        "normal FMIN/FMAX cases failed"
    );

    ASSERT_EDGE(
        requests[2].ro == 0xC0000000
            && requests[3].ro == 0x3F800000,
        "negative FMIN/FMAX cases failed"
    );

    ASSERT_EDGE(
        requests[4].ro == 0x3F800000
            && requests[5].ro == 0x3F800000,
        "infinity FMIN/FMAX cases failed"
    );

    ASSERT_EDGE(
        utils.isNaN(requests[6].ro)
            && utils.isNaN(requests[7].ro),
        "NaN FMIN/FMAX cases failed"
    );
}

int main() {
    test_add_sub_special_cases();
    test_mul_special_cases();
    test_min_max_special_cases();

    std::cout << "[SUCCESS] FP Edge Tests passed!" << std::endl;
    return 0;
}
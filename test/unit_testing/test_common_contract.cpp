#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <type_traits>

#include "test_common.h"

#define ASSERT_COMMON_CONTRACT(cond, msg) do { if (!(cond)) { std::cerr << "[FAIL] test_common_contract: " << msg << " (" << #cond << ")" << std::endl; std::exit(EXIT_FAILURE); } } while (0)

int main() {
    std::cout << "--- Starting Common ABI Contract Tests ---" << std::endl;

    ASSERT_COMMON_CONTRACT(OP_FADD == 8, "FADD opcode must be 8");
    ASSERT_COMMON_CONTRACT(OP_FSUB == 9, "FSUB opcode must be 9");
    ASSERT_COMMON_CONTRACT(OP_FMUL == 10, "FMUL opcode must be 10");
    ASSERT_COMMON_CONTRACT(OP_FMIN == 13, "FMIN opcode must be 13");
    ASSERT_COMMON_CONTRACT(OP_FMAX == 14, "FMAX opcode must be 14");
    ASSERT_COMMON_CONTRACT(OP_FMA  == 15, "FMA opcode must be 15");

    ASSERT_COMMON_CONTRACT(ROUND_NEAREST_TIES_TO_EVEN == 0, "round mode 0 mismatch");
    ASSERT_COMMON_CONTRACT(ROUND_NEAREST_TIES_AWAY_ZERO == 1, "round mode 1 mismatch");
    ASSERT_COMMON_CONTRACT(ROUND_TOWARD_ZERO == 2, "round mode 2 mismatch");
    ASSERT_COMMON_CONTRACT(ROUND_TOWARD_POS_INF == 3, "round mode 3 mismatch");
    ASSERT_COMMON_CONTRACT(ROUND_TOWARD_NEG_INF == 4, "round mode 4 mismatch");

    ASSERT_COMMON_CONTRACT((std::is_same<decltype(Request::r1), uint32_t>::value), "Request::r1 type mismatch");
    ASSERT_COMMON_CONTRACT((std::is_same<decltype(Request::r2), uint32_t>::value), "Request::r2 type mismatch");
    ASSERT_COMMON_CONTRACT((std::is_same<decltype(Request::r3), uint32_t>::value), "Request::r3 type mismatch");
    ASSERT_COMMON_CONTRACT((std::is_same<decltype(Request::ro), uint32_t>::value), "Request::ro type mismatch");
    ASSERT_COMMON_CONTRACT((std::is_same<decltype(Request::op), uint8_t>::value), "Request::op type mismatch");

    Request req = {0x11111111u, 0x22222222u, 0x33333333u, 0x44444444u, OP_FMA};
    ASSERT_COMMON_CONTRACT(req.r1 == 0x11111111u, "Request aggregate order: r1");
    ASSERT_COMMON_CONTRACT(req.r2 == 0x22222222u, "Request aggregate order: r2");
    ASSERT_COMMON_CONTRACT(req.r3 == 0x33333333u, "Request aggregate order: r3");
    ASSERT_COMMON_CONTRACT(req.ro == 0x44444444u, "Request aggregate order: ro");
    ASSERT_COMMON_CONTRACT(req.op == OP_FMA, "Request aggregate order: op");

    ASSERT_COMMON_CONTRACT((std::is_same<decltype(Result::cycles), uint32_t>::value), "Result::cycles type mismatch");
    ASSERT_COMMON_CONTRACT((std::is_same<decltype(Result::zeros), uint32_t>::value), "Result::zeros type mismatch");
    ASSERT_COMMON_CONTRACT((std::is_standard_layout<Request>::value), "Request must be standard-layout");
    ASSERT_COMMON_CONTRACT((std::is_trivially_copyable<Request>::value), "Request must be trivially copyable");
    ASSERT_COMMON_CONTRACT((std::is_standard_layout<Result>::value), "Result must be standard-layout");
    ASSERT_COMMON_CONTRACT((std::is_trivially_copyable<Result>::value), "Result must be trivially copyable");

    std::cout << "[SUCCESS] Common ABI Contract Tests passed!" << std::endl;
    return 0;
}

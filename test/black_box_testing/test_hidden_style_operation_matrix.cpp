#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "fp_ops.h"
#include "fp_utils.h"
#include "test_common.h"

#define ASSERT_OP_MATRIX(cond, msg) do { if (!(cond)) { std::cerr << "[FAIL] test_hidden_style_operation_matrix: " << msg << " (" << #cond << ")" << std::endl; std::exit(EXIT_FAILURE); } } while (0)

static uint32_t exec(uint8_t op, uint32_t a, uint32_t b, uint32_t c, FPUtils& u,
                     bool& z, bool& s, bool& o, bool& uf, bool& ie, bool& n) {
    z = s = o = uf = ie = n = true;
    return FPOps::execute(op, a, b, c, u, ROUND_NEAREST_TIES_TO_EVEN, z, s, o, uf, ie, n);
}

int main() {
    std::cout << "--- Starting Hidden-Style Operation Matrix Tests ---" << std::endl;

    FPUtils u(8, 23);
    bool z, s, o, uf, ie, n;
    const uint32_t pz = 0x00000000u;
    const uint32_t nz = 0x80000000u;
    const uint32_t one = 0x3F800000u;
    const uint32_t negOne = 0xBF800000u;
    const uint32_t two = 0x40000000u;
    const uint32_t three = 0x40400000u;
    const uint32_t negThree = 0xC0400000u;
    const uint32_t pi = u.getPositiveInf();
    const uint32_t ni = u.getNegativeInf();
    const uint32_t nan = u.getNaN();

    ASSERT_OP_MATRIX(exec(OP_FADD, one, two, 0xDEADBEEFu, u, z, s, o, uf, ie, n) == three, "FADD exact finite result mismatch");
    ASSERT_OP_MATRIX(!z && !s && !o && !uf && !ie && !n, "FADD exact finite flags mismatch");
    ASSERT_OP_MATRIX(exec(OP_FSUB, one, two, 0xDEADBEEFu, u, z, s, o, uf, ie, n) == negOne, "FSUB exact finite result mismatch");
    ASSERT_OP_MATRIX(exec(OP_FMUL, two, 0xBFC00000u, 0xDEADBEEFu, u, z, s, o, uf, ie, n) == negThree, "FMUL exact finite result mismatch");

    ASSERT_OP_MATRIX(exec(OP_FADD, pi, pi, 0, u, z, s, o, uf, ie, n) == pi && !n, "+inf + +inf mismatch");
    ASSERT_OP_MATRIX(exec(OP_FADD, ni, ni, 0, u, z, s, o, uf, ie, n) == ni && s && !n, "-inf + -inf mismatch");
    ASSERT_OP_MATRIX(u.isNaN(exec(OP_FADD, pi, ni, 0, u, z, s, o, uf, ie, n)) && n, "+inf + -inf should be NaN");
    ASSERT_OP_MATRIX(u.isNaN(exec(OP_FSUB, pi, pi, 0, u, z, s, o, uf, ie, n)) && n, "+inf - +inf should be NaN");

    ASSERT_OP_MATRIX(exec(OP_FMUL, pz, one, 0, u, z, s, o, uf, ie, n) == pz && z && !s, "+0 * +finite mismatch");
    ASSERT_OP_MATRIX(exec(OP_FMUL, nz, one, 0, u, z, s, o, uf, ie, n) == nz && z && s, "-0 * +finite mismatch");
    ASSERT_OP_MATRIX(exec(OP_FMUL, pz, negOne, 0, u, z, s, o, uf, ie, n) == nz && z && s, "+0 * -finite mismatch");
    ASSERT_OP_MATRIX(exec(OP_FMUL, ni, negOne, 0, u, z, s, o, uf, ie, n) == pi && !n, "-inf * -finite mismatch");
    ASSERT_OP_MATRIX(u.isNaN(exec(OP_FMUL, pi, pz, 0, u, z, s, o, uf, ie, n)) && n, "inf * zero should be NaN");

    ASSERT_OP_MATRIX(exec(OP_FMUL, pz, nan, 0, u, z, s, o, uf, ie, n) == pz && z && !n, "+0 * NaN project rule mismatch");
    ASSERT_OP_MATRIX(exec(OP_FMUL, nz, nan, 0, u, z, s, o, uf, ie, n) == nz && z && s && !n, "-0 * NaN project rule mismatch");
    ASSERT_OP_MATRIX(u.isNaN(exec(OP_FADD, one, nan, 0, u, z, s, o, uf, ie, n)) && n, "FADD NaN propagation mismatch");

    ASSERT_OP_MATRIX(exec(OP_FMIN, one, two, 0, u, z, s, o, uf, ie, n) == one, "FMIN finite mismatch");
    ASSERT_OP_MATRIX(exec(OP_FMAX, one, two, 0, u, z, s, o, uf, ie, n) == two, "FMAX finite mismatch");
    ASSERT_OP_MATRIX(exec(OP_FMIN, nz, pz, 0, u, z, s, o, uf, ie, n) == nz && z && s, "FMIN signed zero mismatch");
    ASSERT_OP_MATRIX(exec(OP_FMAX, nz, pz, 0, u, z, s, o, uf, ie, n) == pz && z && !s, "FMAX signed zero mismatch");
    ASSERT_OP_MATRIX(u.isNaN(exec(OP_FMIN, nan, one, 0, u, z, s, o, uf, ie, n)) && n, "FMIN NaN propagation mismatch");
    ASSERT_OP_MATRIX(u.isNaN(exec(OP_FMAX, one, nan, 0, u, z, s, o, uf, ie, n)) && n, "FMAX NaN propagation mismatch");

    uint32_t invalid = exec(0xFF, one, two, three, u, z, s, o, uf, ie, n);
    ASSERT_OP_MATRIX(invalid == 0u && !z && !s && !o && !uf && !ie && !n, "invalid opcode behavior mismatch");

    std::cout << "[SUCCESS] Hidden-Style Operation Matrix Tests passed!" << std::endl;
    return 0;
}

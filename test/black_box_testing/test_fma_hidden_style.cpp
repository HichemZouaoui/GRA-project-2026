#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "fma_core.h"
#include "fp_utils.h"
#include "test_common.h"

#define ASSERT_FMA_HIDDEN(cond, msg) do { if (!(cond)) { std::cerr << "[FAIL] test_fma_hidden_style: " << msg << " (" << #cond << ")" << std::endl; std::exit(EXIT_FAILURE); } } while (0)

static uint32_t fma(uint32_t a, uint32_t b, uint32_t c, uint8_t mode, FPUtils& u,
                    bool& z, bool& s, bool& o, bool& uf, bool& ie, bool& n) {
    z = s = o = uf = ie = n = true;
    return FMACore::executeFMA(a, b, c, u, mode, z, s, o, uf, ie, n);
}

int main() {
    std::cout << "--- Starting Hidden-Style FMA Tests ---" << std::endl;

    FPUtils u(8, 23);
    bool z, s, o, uf, ie, n;
    const uint32_t pz = 0x00000000u;
    const uint32_t nz = 0x80000000u;
    const uint32_t one = 0x3F800000u;
    const uint32_t negOne = 0xBF800000u;
    const uint32_t two = 0x40000000u;
    const uint32_t three = 0x40400000u;
    const uint32_t pi = u.getPositiveInf();
    const uint32_t ni = u.getNegativeInf();
    const uint32_t nan = u.getNaN();
    const uint32_t twoMinus24 = 0x33800000u;

    ASSERT_FMA_HIDDEN(fma(one, two, three, ROUND_NEAREST_TIES_TO_EVEN, u, z, s, o, uf, ie, n) == 0x40E00000u, "basic 1 + 2*3 mismatch");
    ASSERT_FMA_HIDDEN(!z && !s && !o && !uf && !ie && !n, "basic exact FMA flags mismatch");

    ASSERT_FMA_HIDDEN(fma(negOne, one, one, ROUND_NEAREST_TIES_TO_EVEN, u, z, s, o, uf, ie, n) == pz, "-1 + 1*1 should be +0");
    ASSERT_FMA_HIDDEN(z && !s && !n, "zero cancellation flags mismatch");

    ASSERT_FMA_HIDDEN(fma(nz, nz, nan, ROUND_NEAREST_TIES_TO_EVEN, u, z, s, o, uf, ie, n) == nz, "-0 + -0*NaN project rule mismatch");
    ASSERT_FMA_HIDDEN(z && s && !n, "-0 + -0*NaN flags mismatch");
    ASSERT_FMA_HIDDEN(fma(one, pz, nan, ROUND_NEAREST_TIES_TO_EVEN, u, z, s, o, uf, ie, n) == one, "1 + +0*NaN project rule mismatch");
    ASSERT_FMA_HIDDEN(!z && !s && !n, "1 + +0*NaN flags mismatch");

    ASSERT_FMA_HIDDEN(u.isNaN(fma(one, pi, pz, ROUND_NEAREST_TIES_TO_EVEN, u, z, s, o, uf, ie, n)) && n, "finite + inf*0 should be NaN");
    ASSERT_FMA_HIDDEN(u.isNaN(fma(pi, ni, one, ROUND_NEAREST_TIES_TO_EVEN, u, z, s, o, uf, ie, n)) && n, "+inf + -inf*1 should be NaN");
    ASSERT_FMA_HIDDEN(fma(pi, pi, one, ROUND_NEAREST_TIES_TO_EVEN, u, z, s, o, uf, ie, n) == pi && !n, "+inf + +inf*1 should be +inf");

    // This catches implementations that round r2*r3 before adding r1.

    ASSERT_FMA_HIDDEN(fma(0xBF800000u, 0x3F800001u, 0x3F7FFFFFu, ROUND_NEAREST_TIES_TO_EVEN, u, z, s, o, uf, ie, n) == 0x337FFFFEu,
                      "fused precision cancellation result mismatch");

    // FMA must apply the selected final rounding mode.
    ASSERT_FMA_HIDDEN(fma(one, one, twoMinus24, ROUND_NEAREST_TIES_TO_EVEN, u, z, s, o, uf, ie, n) == 0x3F800000u,
                      "FMA nearest-even positive half-ULP mismatch");
    ASSERT_FMA_HIDDEN(fma(one, one, twoMinus24, ROUND_NEAREST_TIES_AWAY_ZERO, u, z, s, o, uf, ie, n) == 0x3F800001u,
                      "FMA nearest-away positive half-ULP mismatch");
    ASSERT_FMA_HIDDEN(fma(one, one, twoMinus24, ROUND_TOWARD_POS_INF, u, z, s, o, uf, ie, n) == 0x3F800001u,
                      "FMA toward +inf positive half-ULP mismatch");
    ASSERT_FMA_HIDDEN(fma(one, one, twoMinus24, ROUND_TOWARD_NEG_INF, u, z, s, o, uf, ie, n) == 0x3F800000u,
                      "FMA toward -inf positive half-ULP mismatch");

    std::cout << "[SUCCESS] Hidden-Style FMA Tests passed!" << std::endl;
    return 0;
}

#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "fp_ops.h"
#include "fp_utils.h"
#include "simulation.h"
#include "test_common.h"

#define ASSERT_SUBNORMAL(cond, msg) do { if (!(cond)) { std::cerr << "[FAIL] test_subnormal_contract: " << msg << " (" << #cond << ")" << std::endl; std::exit(EXIT_FAILURE); } } while (0)

static Request mk(uint8_t op, uint32_t a, uint32_t b, uint32_t c = 0) {
    Request r = {a, b, c, 0, op};
    return r;
}

static uint32_t exec(uint8_t op, uint32_t a, uint32_t b, uint32_t c, FPUtils& u,
                     bool& z, bool& s, bool& o, bool& uf, bool& ie, bool& n) {
    z = s = o = uf = ie = n = false;
    return FPOps::execute(op, a, b, c, u, ROUND_NEAREST_TIES_TO_EVEN, z, s, o, uf, ie, n);
}

int main() {
    std::cout << "--- Starting Subnormal Arithmetic Contract Tests ---" << std::endl;

    FPUtils u(8, 23);
    bool z, s, o, uf, ie, n;

    const uint32_t tiny       = 0x00000001u; // smallest positive subnormal, 2^-149
    const uint32_t tiny2      = 0x00000002u;
    const uint32_t largestSub = 0x007FFFFFu;
    const uint32_t minNormal  = 0x00800000u; // 2^-126
    const uint32_t halfMin    = 0x00400000u; // 2^-127, subnormal
    const uint32_t two        = 0x40000000u;
    const uint32_t half       = 0x3F000000u;
    const uint32_t negTiny    = 0x80000001u;

    uint32_t r = exec(OP_FADD, tiny, tiny, 0, u, z, s, o, uf, ie, n);
    ASSERT_SUBNORMAL(r == tiny2, "smallest subnormal + itself should be second subnormal");
    ASSERT_SUBNORMAL(!z && !s && !o && !n, "subnormal addition flags should not report zero/sign/overflow/nan");
    ASSERT_SUBNORMAL(uf, "subnormal non-zero result should raise underflow flag");

    r = exec(OP_FSUB, minNormal, largestSub, 0, u, z, s, o, uf, ie, n);
    ASSERT_SUBNORMAL(r == tiny, "min normal - largest subnormal should be smallest subnormal");
    ASSERT_SUBNORMAL(!z && !s && uf && !o && !n, "subnormal subtraction flags mismatch");

    r = exec(OP_FMUL, minNormal, half, 0, u, z, s, o, uf, ie, n);
    ASSERT_SUBNORMAL(r == halfMin, "min normal * 0.5 should produce exact subnormal half-min");
    ASSERT_SUBNORMAL(!z && !s && uf && !o && !n, "subnormal multiplication flags mismatch");

    r = exec(OP_FMUL, tiny, two, 0, u, z, s, o, uf, ie, n);
    ASSERT_SUBNORMAL(r == tiny2, "smallest subnormal * 2.0 should be second subnormal");
    ASSERT_SUBNORMAL(!z && !s && uf && !o && !n, "tiny*2 flags mismatch");

    r = exec(OP_FADD, tiny, negTiny, 0, u, z, s, o, uf, ie, n);
    ASSERT_SUBNORMAL(r == 0x00000000u, "tiny + (-tiny) should cancel to +0");
    ASSERT_SUBNORMAL(z && !s && !o && !uf && !ie && !n, "exact subnormal cancellation flags mismatch");

    Request reqs[5];
    reqs[0] = mk(OP_FADD, tiny, tiny);
    reqs[1] = mk(OP_FSUB, minNormal, largestSub);
    reqs[2] = mk(OP_FMUL, minNormal, half);
    reqs[3] = mk(OP_FMUL, tiny, two);
    reqs[4] = mk(OP_FADD, tiny, negTiny);

    Result sim = runSimulation(5, nullptr, 8, 23, ROUND_NEAREST_TIES_TO_EVEN, 5, reqs);
    ASSERT_SUBNORMAL(sim.cycles == 5, "simulation cycles mismatch");
    ASSERT_SUBNORMAL(reqs[0].ro == tiny2 && reqs[1].ro == tiny && reqs[2].ro == halfMin && reqs[3].ro == tiny2 && reqs[4].ro == 0x00000000u, "simulation subnormal results mismatch");
    ASSERT_SUBNORMAL(sim.underflows >= 4, "simulation underflow count should include non-zero subnormal results");
    ASSERT_SUBNORMAL(sim.zeros >= 1, "simulation zero count should include exact cancellation");

    std::cout << "[SUCCESS] Subnormal Arithmetic Contract Tests passed!" << std::endl;
    return 0;
}

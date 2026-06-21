#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "fp_ops.h"
#include "fp_utils.h"
#include "simulation.h"
#include "test_common.h"

#define ASSERT_SZERO(cond, msg) do { if (!(cond)) { std::cerr << "[FAIL] test_signed_zero_and_flags_contract: " << msg << " (" << #cond << ")" << std::endl; std::exit(EXIT_FAILURE); } } while (0)

static Request mk(uint8_t op, uint32_t a, uint32_t b, uint32_t c = 0) { Request r = {a, b, c, 0, op}; return r; }

static uint32_t exec(uint8_t op, uint32_t a, uint32_t b, uint32_t c, FPUtils& u,
                     bool& z, bool& s, bool& o, bool& uf, bool& ie, bool& n) {

    // Initialize to true to catch stale flags not being reset by the operation.

    z = s = o = uf = ie = n = true;
    return FPOps::execute(op, a, b, c, u, ROUND_NEAREST_TIES_TO_EVEN, z, s, o, uf, ie, n);
}

int main() {
    std::cout << "--- Starting Signed Zero and Stale Flag Contract Tests ---" << std::endl;

    FPUtils u(8, 23);
    bool z, s, o, uf, ie, n;

    const uint32_t pz = 0x00000000u;
    const uint32_t nz = 0x80000000u;
    const uint32_t one = 0x3F800000u;
    const uint32_t negOne = 0xBF800000u;

    uint32_t r = exec(OP_FADD, pz, pz, 0, u, z, s, o, uf, ie, n);
    ASSERT_SZERO(r == pz && z && !s && !o && !uf && !ie && !n, "+0 + +0 should be clean +0");

    r = exec(OP_FADD, nz, nz, 0, u, z, s, o, uf, ie, n);
    ASSERT_SZERO(r == nz && z && s && !o && !uf && !ie && !n, "-0 + -0 should preserve -0");

    r = exec(OP_FADD, pz, nz, 0, u, z, s, o, uf, ie, n);
    ASSERT_SZERO(r == pz && z && !s && !o && !uf && !ie && !n, "+0 + -0 should be +0 in nearest-even mode");

    r = exec(OP_FSUB, pz, pz, 0, u, z, s, o, uf, ie, n);
    ASSERT_SZERO(r == pz && z && !s && !o && !uf && !ie && !n, "+0 - +0 should be +0");

    r = exec(OP_FSUB, nz, pz, 0, u, z, s, o, uf, ie, n);
    ASSERT_SZERO(r == nz && z && s && !o && !uf && !ie && !n, "-0 - +0 should be -0");

    r = exec(OP_FSUB, pz, nz, 0, u, z, s, o, uf, ie, n);
    ASSERT_SZERO(r == pz && z && !s && !o && !uf && !ie && !n, "+0 - -0 should be +0");

    r = exec(OP_FMUL, pz, negOne, 0, u, z, s, o, uf, ie, n);
    ASSERT_SZERO(r == nz && z && s && !o && !uf && !ie && !n, "+0 * -1 should be -0");

    r = exec(OP_FMUL, nz, negOne, 0, u, z, s, o, uf, ie, n);
    ASSERT_SZERO(r == pz && z && !s && !o && !uf && !ie && !n, "-0 * -1 should be +0");

    r = exec(OP_FMIN, nz, pz, 0, u, z, s, o, uf, ie, n);
    ASSERT_SZERO(r == nz && z && s && !o && !uf && !ie && !n, "FMIN(-0,+0) should be -0");

    r = exec(OP_FMAX, nz, pz, 0, u, z, s, o, uf, ie, n);
    ASSERT_SZERO(r == pz && z && !s && !o && !uf && !ie && !n, "FMAX(-0,+0) should be +0");

    r = exec(OP_FADD, one, negOne, 0, u, z, s, o, uf, ie, n);
    ASSERT_SZERO(r == pz && z && !s && !o && !uf && !ie && !n, "1 + -1 should be clean +0 and clear stale flags");

    Request reqs[4];
    reqs[0] = mk(OP_FADD, nz, nz);
    reqs[1] = mk(OP_FMUL, pz, negOne);
    reqs[2] = mk(OP_FMIN, nz, pz);
    reqs[3] = mk(OP_FMAX, nz, pz);
    Result sim = runSimulation(4, nullptr, 8, 23, ROUND_NEAREST_TIES_TO_EVEN, 4, reqs);
    ASSERT_SZERO(sim.cycles == 4, "simulation cycle mismatch");
    ASSERT_SZERO(reqs[0].ro == nz && reqs[1].ro == nz && reqs[2].ro == nz && reqs[3].ro == pz, "simulation signed-zero results mismatch");
    ASSERT_SZERO(sim.zeros == 4, "simulation should count all signed zero results as zero");
    ASSERT_SZERO(sim.signs == 3, "simulation sign count mismatch for signed zero results");

    std::cout << "[SUCCESS] Signed Zero and Stale Flag Contract Tests passed!" << std::endl;
    return 0;
}

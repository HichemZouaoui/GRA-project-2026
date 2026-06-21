#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "simulation.h"
#include "test_common.h"

#define ASSERT_CUSTOM_FMT(cond, msg) do { if (!(cond)) { std::cerr << "[FAIL] test_custom_format_operations: " << msg << " (" << #cond << ")" << std::endl; std::exit(EXIT_FAILURE); } } while (0)

static Request mk(uint8_t op, uint32_t a, uint32_t b, uint32_t c = 0) {
    Request r = {a, b, c, 0, op};
    return r;
}

int main() {
    std::cout << "--- Starting Custom Format Operation Tests ---" << std::endl;

    // 5 exponent bits, 10 mantissa bits: half-like layout.
    // 1.0=0x3C00, 0.5=0x3800, 1.5=0x3E00, 2.0=0x4000, 3.0=0x4200, 4.0=0x4400, 7.0=0x4700

    Request h[8];
    h[0] = mk(OP_FADD, 0x3C00u, 0x3800u);       // 1.0 + 0.5 = 1.5
    h[1] = mk(OP_FSUB, 0x4000u, 0x3C00u);       // 2.0 - 1.0 = 1.0
    h[2] = mk(OP_FMUL, 0x3E00u, 0x4000u);       // 1.5 * 2.0 = 3.0
    h[3] = mk(OP_FMIN, 0xBC00u, 0x3C00u);       // min(-1.0, 1.0) = -1.0
    h[4] = mk(OP_FMAX, 0xBC00u, 0x3C00u);       // max(-1.0, 1.0) = 1.0
    h[5] = mk(OP_FMA,  0x3C00u, 0x4000u, 0x4200u); // 1.0 + 2.0*3.0 = 7.0
    h[6] = mk(OP_FMUL, 0x7C00u, 0x0000u);       // inf * 0 = NaN
    h[7] = mk(OP_FADD, 0x7BFFu, 0x7BFFu);       // max + max = overflow

    Result rh = runSimulation(8, nullptr, 5, 10, ROUND_NEAREST_TIES_TO_EVEN, 8, h);
    ASSERT_CUSTOM_FMT(rh.cycles == 8, "half-like cycle count mismatch");
    ASSERT_CUSTOM_FMT(h[0].ro == 0x3E00u, "half-like FADD mismatch");
    ASSERT_CUSTOM_FMT(h[1].ro == 0x3C00u, "half-like FSUB mismatch");
    ASSERT_CUSTOM_FMT(h[2].ro == 0x4200u, "half-like FMUL mismatch");
    ASSERT_CUSTOM_FMT(h[3].ro == 0xBC00u, "half-like FMIN mismatch");
    ASSERT_CUSTOM_FMT(h[4].ro == 0x3C00u, "half-like FMAX mismatch");
    ASSERT_CUSTOM_FMT(h[5].ro == 0x4700u, "half-like FMA mismatch");
    ASSERT_CUSTOM_FMT((h[6].ro & 0x7C00u) == 0x7C00u && (h[6].ro & 0x03FFu) != 0u, "half-like NaN mismatch");
    ASSERT_CUSTOM_FMT(h[7].ro == 0x7C00u, "half-like overflow result mismatch");
    ASSERT_CUSTOM_FMT(rh.nans >= 1 && rh.overflows >= 1, "half-like flags mismatch");

    // 4 exponent bits, 8 mantissa bits: tests arbitrary custom widths, not only 8/23 and 5/10.

    Request c[4];
    c[0] = mk(OP_FADD, 0x0700u, 0x0600u);       // 1.0 + 0.5 = 1.5 => exp 7 mant 0x80 => 0x0780
    c[1] = mk(OP_FMUL, 0x0780u, 0x0800u);       // 1.5 * 2.0 = 3.0 => 0x0880
    c[2] = mk(OP_FMA,  0x0700u, 0x0800u, 0x0880u); // 1.0 + 2.0*3.0 = 7.0 => 0x09C0
    c[3] = mk(OP_FADD, 0x0EFFu, 0x0EFFu);       // max + max = +inf

    Result rc = runSimulation(4, nullptr, 4, 8, ROUND_NEAREST_TIES_TO_EVEN, 4, c);
    ASSERT_CUSTOM_FMT(rc.cycles == 4, "4/8 cycle count mismatch");
    ASSERT_CUSTOM_FMT(c[0].ro == 0x0780u, "4/8 FADD mismatch");
    ASSERT_CUSTOM_FMT(c[1].ro == 0x0880u, "4/8 FMUL mismatch");
    ASSERT_CUSTOM_FMT(c[2].ro == 0x09C0u, "4/8 FMA mismatch");
    ASSERT_CUSTOM_FMT(c[3].ro == 0x0F00u, "4/8 overflow mismatch");
    ASSERT_CUSTOM_FMT(rc.overflows >= 1, "4/8 overflow flag mismatch");

    std::cout << "[SUCCESS] Custom Format Operation Tests passed!" << std::endl;
    return 0;
}

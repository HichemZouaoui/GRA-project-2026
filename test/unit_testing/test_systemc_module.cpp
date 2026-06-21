#include <systemc.h>

#include <cstdlib>
#include <cstdint>
#include <iostream>

#include "test_systemc_module.h"
#include "test_common.h"

#define ASSERT_SYSTEMC(cond, msg)
    do {
        if (!(cond)) {
            std::cerr << "[FAIL] test_systemc_module: " << msg
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    } while (0)

static sc_bv<4> opcodeToBitVector(uint8_t opcode) {
    sc_bv<4> bits;
    bits = static_cast<unsigned long>(opcode & 0x0F);
    return bits;
}

int sc_main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    sc_clock clk("clk", 10, SC_NS);

    sc_signal<uint32_t> r1("r1");
    sc_signal<uint32_t> r2("r2");
    sc_signal<uint32_t> r3("r3");
    sc_signal<uint32_t> ro("ro");

    sc_signal<sc_bv<4>> op("op");

    sc_signal<bool> zero("zero");
    sc_signal<bool> sign("sign");
    sc_signal<bool> overflow("overflow");
    sc_signal<bool> underflow("underflow");
    sc_signal<bool> inexact("inexact");
    sc_signal<bool> nan("nan");

    FLOATING_POINT_UNIT uut(
        "FPU_UUT",
        8,
        23,
        ROUND_NEAREST_TIES_TO_EVEN
    );

    uut.clk(clk);
    uut.r1(r1);
    uut.r2(r2);
    uut.r3(r3);
    uut.op(op);
    uut.ro(ro);
    uut.zero(zero);
    uut.sign(sign);
    uut.overflow(overflow);
    uut.underflow(underflow);
    uut.inexact(inexact);
    uut.nan(nan);

    // Cycle 1: FADD
    // 1.0 + 2.0 = 3.0
    // r3 contains junk and must be ignored.

    op.write(opcodeToBitVector(OP_FADD));
    r1.write(0x3F800000);
    r2.write(0x40000000);
    r3.write(0xDEADBEEF);

    sc_start(10, SC_NS);

    ASSERT_SYSTEMC(
        ro.read() == 0x40400000,
        "FADD result mismatch"
    );

    ASSERT_SYSTEMC(
        !zero.read()
            && !sign.read()
            && !overflow.read()
            && !underflow.read()
            && !inexact.read()
            && !nan.read(),
        "FADD flags should all be inactive"
    );

    // Cycle 2: zero flag
    // 1.0 + (-1.0) = +0.0

    r1.write(0x3F800000);
    r2.write(0xBF800000);

    sc_start(10, SC_NS);

    ASSERT_SYSTEMC(
        ro.read() == 0x00000000 && zero.read(),
        "zero result or zero flag mismatch"
    );

    // Cycle 3: sign flag
    // -1.0 + (-2.0) = -3.0

    r1.write(0xBF800000);
    r2.write(0xC0000000);

    sc_start(10, SC_NS);

    ASSERT_SYSTEMC(
        ro.read() == 0xC0400000
            && sign.read()
            && !zero.read(),
        "negative result or sign flag mismatch"
    );

    // Cycle 4: FMA
    // 1.0 + (2.0 * 3.0) = 7.0

    op.write(opcodeToBitVector(OP_FMA));
    r1.write(0x3F800000);
    r2.write(0x40000000);
    r3.write(0x40400000);

    sc_start(10, SC_NS);

    ASSERT_SYSTEMC(
        ro.read() == 0x40E00000 && !nan.read(),
        "FMA result mismatch"
    );

    // Cycle 5: FMA exceptional case
    // 1.0 + (+INF * +0.0) = NaN

    op.write(opcodeToBitVector(OP_FMA));
    r1.write(0x3F800000);
    r2.write(0x7F800000);
    r3.write(0x00000000);

    sc_start(10, SC_NS);

    ASSERT_SYSTEMC(
        nan.read(),
        "FMA INF * 0 should raise NaN flag"
    );

    std::cout << "[SUCCESS] SystemC FPU Module integration tests passed!"
              << std::endl;

    return 0;
}
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <systemc.h>

#include "test_common.h"
#include "test_systemc_module.h"

#define ASSERT_SC_FULL(cond, msg) do { if (!(cond)) { std::cerr << "[FAIL] test_systemc_full_ops_contract: " << msg << " (" << #cond << ")" << std::endl; std::exit(EXIT_FAILURE); } } while (0)

static sc_bv<4> opbv(uint8_t op) {
    sc_bv<4> v;
    v = op & 0xFu;
    return v;
}

int sc_main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    std::cout << "--- Starting SystemC Full Operation Contract Tests ---" << std::endl;

    sc_clock clk("clk", 10, SC_NS);
    sc_signal<uint32_t> r1("r1"), r2("r2"), r3("r3"), ro("ro");
    sc_signal<sc_bv<4>> op("op");
    sc_signal<bool> zero("zero"), sign("sign"), overflow("overflow"), underflow("underflow"), inexact("inexact"), nan("nan");

    FLOATING_POINT_UNIT uut("FPU_FULL_CONTRACT", 8, 23, ROUND_NEAREST_TIES_TO_EVEN);
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

    op.write(opbv(OP_FADD)); r1.write(0x3F800000u); r2.write(0x40000000u); r3.write(0xDEADBEEFu);
    sc_start(10, SC_NS);
    ASSERT_SC_FULL(ro.read() == 0x40400000u, "FADD result mismatch");
    ASSERT_SC_FULL(!zero.read() && !sign.read() && !overflow.read() && !underflow.read() && !inexact.read() && !nan.read(), "FADD flags mismatch");

    op.write(opbv(OP_FSUB)); r1.write(0x3F800000u); r2.write(0x40000000u); r3.write(0xDEADBEEFu);
    sc_start(10, SC_NS);
    ASSERT_SC_FULL(ro.read() == 0xBF800000u, "FSUB result mismatch");
    ASSERT_SC_FULL(!zero.read() && sign.read() && !overflow.read() && !underflow.read() && !inexact.read() && !nan.read(), "FSUB flags mismatch or stale flag leaked");

    op.write(opbv(OP_FMUL)); r1.write(0x40000000u); r2.write(0x40400000u); r3.write(0xDEADBEEFu);
    sc_start(10, SC_NS);
    ASSERT_SC_FULL(ro.read() == 0x40C00000u, "FMUL result mismatch");
    ASSERT_SC_FULL(!zero.read() && !sign.read() && !overflow.read() && !underflow.read() && !inexact.read() && !nan.read(), "FMUL flags mismatch or stale sign leaked");

    op.write(opbv(OP_FMIN)); r1.write(0xC0000000u); r2.write(0x3F800000u); r3.write(0xDEADBEEFu);
    sc_start(10, SC_NS);
    ASSERT_SC_FULL(ro.read() == 0xC0000000u, "FMIN result mismatch");
    ASSERT_SC_FULL(!zero.read() && sign.read() && !overflow.read() && !underflow.read() && !inexact.read() && !nan.read(), "FMIN flags mismatch");

    op.write(opbv(OP_FMAX)); r1.write(0xC0000000u); r2.write(0x3F800000u); r3.write(0xDEADBEEFu);
    sc_start(10, SC_NS);
    ASSERT_SC_FULL(ro.read() == 0x3F800000u, "FMAX result mismatch");
    ASSERT_SC_FULL(!zero.read() && !sign.read() && !overflow.read() && !underflow.read() && !inexact.read() && !nan.read(), "FMAX flags mismatch or stale sign leaked");

    op.write(opbv(OP_FMA)); r1.write(0x3F800000u); r2.write(0x40000000u); r3.write(0x40400000u);
    sc_start(10, SC_NS);
    ASSERT_SC_FULL(ro.read() == 0x40E00000u, "FMA result mismatch");
    ASSERT_SC_FULL(!zero.read() && !sign.read() && !overflow.read() && !underflow.read() && !inexact.read() && !nan.read(), "FMA flags mismatch");

    op.write(opbv(OP_FMUL)); r1.write(0x7F800000u); r2.write(0x00000000u); r3.write(0xDEADBEEFu);
    sc_start(10, SC_NS);
    ASSERT_SC_FULL((ro.read() & 0x7F800000u) == 0x7F800000u && (ro.read() & 0x007FFFFFu) != 0u, "FMUL inf*zero should produce NaN");
    ASSERT_SC_FULL(nan.read() && !zero.read() && !overflow.read() && !underflow.read(), "NaN flag mismatch");

    op.write(opbv(OP_FADD)); r1.write(0x3F800000u); r2.write(0xBF800000u); r3.write(0xDEADBEEFu);
    sc_start(10, SC_NS);
    ASSERT_SC_FULL(ro.read() == 0x00000000u, "FADD cancellation after NaN should produce +0");
    ASSERT_SC_FULL(zero.read() && !sign.read() && !overflow.read() && !underflow.read() && !inexact.read() && !nan.read(), "flags were not fully reset after previous NaN cycle");

    std::cout << "[SUCCESS] SystemC Full Operation Contract Tests passed!" << std::endl;
    return 0;
}

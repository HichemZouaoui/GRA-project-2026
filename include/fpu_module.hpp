#ifndef FPU_MODULE_HPP
#define FPU_MODULE_HPP

#include <systemc>
#include <systemc.h>
#include <stdint.h>
using namespace sc_core;

SC_MODULE(FLOATING_POINT_UNIT) {
    sc_in<bool> clk;
    sc_in<uint32_t> r1;
    sc_in<uint32_t> r2;
    sc_in<uint32_t> r3;
    sc_in<sc_bv<4>> op;

    sc_out<uint32_t> ro;
    sc_out<bool> zero;
    sc_out<bool> sign;
    sc_out<bool> overflow;
    sc_out<bool> underflow;
    sc_out<bool> inexact;
    sc_out<bool> nan;

    SC_CTOR(FLOATING_POINT_UNIT) {
        SC_THREAD(behaviour);
        sensitive << clk.pos();
    }

    void behaviour();

private: 
    static const uint8_t OP_FADD = 8;
    static const uint8_t OP_FSUB = 9;
    static const uint8_t OP_FMUL = 10;
    static const uint8_t OP_FMIN = 13;
    static const uint8_t OP_FMAX = 14;
    static const uint8_t OP_FMA = 15;
};

#endif
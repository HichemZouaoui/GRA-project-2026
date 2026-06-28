#ifndef FPU_MODULE_HPP
#define FPU_MODULE_HPP

#include <cstdint>
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

    SC_HAS_PROCESS(FLOATING_POINT_UNIT);

    FLOATING_POINT_UNIT(
        sc_module_name name,
        uint8_t sizeExponent,
        uint8_t sizeMantissa,
        uint8_t roundMode
    );

    void behaviour();

    uint32_t getPositiveInf();
    uint32_t getNegativeInf();
    double getMax();
    double getMin();

private: 
    uint8_t sizeExponent;
    uint8_t sizeMantissa;
    uint8_t roundMode;
};

#endif
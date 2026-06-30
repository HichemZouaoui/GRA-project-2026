#include "fpu_module.hpp"

#include "fp_ops.h"
#include "fp_utils.h"
#include <cstdint>


FLOATING_POINT_UNIT::FLOATING_POINT_UNIT(
    sc_module_name name,
    uint8_t sizeExponent,
    uint8_t sizeMantissa,
    uint8_t roundMode
) : sc_module(name),
sizeExponent(sizeExponent),
sizeMantissa(sizeMantissa),
roundMode(roundMode) {
    SC_THREAD(behaviour);
    sensitive << clk.pos();
}


void FLOATING_POINT_UNIT::behaviour() {
    while(true) {
        wait();

        uint32_t input1 = r1.read();
        uint32_t input2 = r2.read();
        uint32_t input3 = r3.read();
        uint32_t currentOp = op.read().to_uint();

        bool zeroFlag = false;
        bool signFlag = false;
        bool overflowFlag = false;
        bool underflowFlag = false;
        bool inexactFlag = false;
        bool nanFlag = false;

        FPUtils utils(sizeExponent, sizeMantissa);

        uint32_t result = FPOps::execute(
            currentOp,
            input1,
            input2,
            input3,
            utils,
            roundMode,
            zeroFlag,
            signFlag,
            overflowFlag,
            underflowFlag,
            inexactFlag,
            nanFlag
        );
        ro.write(result);

        zero.write(zeroFlag);
        sign.write(signFlag);
        overflow.write(overflowFlag);
        underflow.write(underflowFlag);
        inexact.write(inexactFlag);
        nan.write(nanFlag);
    }
}

uint32_t FLOATING_POINT_UNIT::getPositiveInf() {
            FPUtils utils(sizeExponent, sizeMantissa);
            return utils.getPositiveInf();
        }

        uint32_t FLOATING_POINT_UNIT::getNegativeInf() {
            FPUtils utils(sizeExponent, sizeMantissa);
            return utils.getNegativeInf();
        }

        double FLOATING_POINT_UNIT::getMax() {
            FPUtils utils(sizeExponent, sizeMantissa);
            return utils.getMax();
        }

        double FLOATING_POINT_UNIT::getMin() {
            FPUtils utils(sizeExponent, sizeMantissa);
            return utils.getMin();
        }
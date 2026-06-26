#include "fpu_module.hpp"
#include "fp_ops.h"

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
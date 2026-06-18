#include "fpu_module.hpp"

void FLOATING_POINT_UNIT::behaviour() {
    while(true) {
        wait();

        uint32_t input1 = r1.read();
        uint32_t input2 = r2.read();
        uint32_t currentOp = op.read().to_uint();

        uint32_t result = 0;

        switch(currentOp) {
            case OP_FADD:
                result = input1 + input2;
                break;

            default:
                result = 0;
                break;
        }
        ro.write(result);

        zero.write(result == 0);
        sign.write((result >> 31) & 1);
        overflow.write(false);
        underflow.write(false);
        inexact.write(false);
        nan.write(false);
    }
}
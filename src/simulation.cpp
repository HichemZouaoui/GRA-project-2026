#include "simulation.h"
#include "fp_ops.h"
#include <cstdint>

Result runSimulation(
    uint32_t cycles,
    const char* tracefile,
    uint8_t sizeExponent,
    uint8_t sizeMantissa,
    uint8_t roundMode,
    uint32_t numRequests,
    Request* requests
) {
    (void)tracefile;

    Result result;
    result.cycles = cycles;
    result.zeros = 0;
    result.signs = 0;
    result.overflows = 0;
    result.underflows = 0;
    result.inexacts = 0;
    result.nans = 0;

    FPUtils utils(sizeExponent, sizeMantissa);

    for (uint32_t i = 0; i < numRequests; i++) {
        bool zero = false;
        bool sign = false;
        bool overflow = false;
        bool underflow = false;
        bool inexact = false;
        bool nan = false;

        uint32_t ro = FPOps::execute(
            requests[i].op,
            requests[i].r1,
            requests[i].r2,
            requests[i].r3,
            utils,
            roundMode,
            zero,
            sign,
            overflow,
            underflow,
            inexact,
            nan
        );

        requests[i].ro = ro;

        if (zero) {
            result.zeros++;
        }
        if (sign) {
            result.signs++;
        }
        if (overflow) {
            result.overflows++;
        }
        if (underflow) {
            result.underflows++;
        }
        if (inexact) {
            result.inexacts++;
        }
        if (nan) {
            result.nans++;
        }
    }
    return result;
}
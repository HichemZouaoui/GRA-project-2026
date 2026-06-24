#include "simulation.h"

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
    (void)sizeExponent;
    (void)sizeMantissa;
    (void)roundMode;

    Result result;
    result.cycles = cycles;
    result.zeros = 0;
    result.signs = 0;
    result.overflows = 0;
    result.underflows = 0;
    result.inexacts = 0;
    result.nans = 0;

    for (uint32_t i = 0; i < numRequests; i++) {
        uint32_t ro = 0;

        switch(requests[i].op) {
            case 8:
            ro = requests[i].r1 + requests[i].r2;
            break;

            case 9:
            ro = requests[i].r1 - requests[i].r2;
            break;

            case 10:
            ro = requests[i].r1 * requests[i].r2;
            break;

            case 13:
            if (requests[i].r1 < requests[i].r2) {
                ro = requests[i].r1;
            }
            else{
                ro = requests[i].r2;
            }
            break;

            case 14:
            if (requests[i].r1 > requests[i].r2) {
                ro = requests[i].r1;
            }
            else{
                ro = requests[i].r2;
            }
            break;

            case 15:
            ro = requests[i].r1 + (requests[i].r2 * requests[i].r3);
            break;

            default: 
            ro = 0;
            break;
        }

        requests[i].ro = ro;

        if (ro == 0){
            result.zeros++;
        }

        if (((ro >> 31) & 1) != 0) {
            result.signs++;
        }
    }
    return result;
}
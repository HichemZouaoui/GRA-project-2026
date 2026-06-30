#ifndef SIMULATION_H
#define SIMULATION_H

#include <stdint.h>

struct Request {
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t ro;
    uint8_t op;
};

struct Result {
    uint32_t cycles;
    uint32_t zeros;
    uint32_t signs;
    uint32_t overflows;
    uint32_t underflows;
    uint32_t inexacts;
    uint32_t nans;
};

Result runSimulation(
    uint32_t cycles,
    const char* tracefile,
    uint8_t sizeExponent,
    uint8_t sizeMantissa,
    uint8_t roundMode,
    uint32_t numRequests,
    Request* requests
);

#endif
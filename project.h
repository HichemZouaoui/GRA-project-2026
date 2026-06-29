#ifndef PROJECT_H
#define PROJECT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    OP_FADD = 8,
    OP_FSUB = 9,
    OP_FMUL = 10,
    OP_FMIN = 13,
    OP_FMAX = 14,
    OP_FMA  = 15
};

enum {
    ROUND_NEAREST_TIES_TO_EVEN   = 0,
    ROUND_NEAREST_TIES_AWAY_ZERO = 1,
    ROUND_TOWARD_ZERO            = 2,
    ROUND_TOWARD_POS_INF         = 3,
    ROUND_TOWARD_NEG_INF         = 4
};

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

struct Result runSimulation(
    uint32_t cycles,
    const char* tracefile,
    uint8_t sizeExponent,
    uint8_t sizeMantissa,
    uint8_t roundMode,
    uint32_t numRequests,
    struct Request* requests
);

struct CliOptions {
    uint32_t cycles;
    char* tracefile;
    uint8_t size_exponent;
    uint8_t size_mantissa;
    uint8_t round_mode;
    char* input_file;
};

void parse_cli_arguments(int argc, char* argv[], struct CliOptions* options);
struct Request* parse_csv_file(const char* filename, uint32_t* num_requests, uint8_t size_exp, uint8_t size_mant);
uint32_t parse_operand(const char* operand_str, uint8_t size_exp, uint8_t size_mant);
void print_simulation_results(const struct Result* result);

#ifdef __cplusplus
}
#endif

#endif

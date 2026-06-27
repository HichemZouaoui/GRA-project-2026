#ifndef PROJECT_H
#define PROJECT_H
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
#ifdef __cplusplus
extern "C" {
#endif
struct Result runSimulation(
    uint32_t cycles,
    const char* tracefile, // Passes NULL if --tf wasn't provided!
    uint8_t sizeExponent,
    uint8_t sizeMantissa,
    uint8_t roundMode,
    uint32_t numRequests,
    struct Request* requests
);
#ifdef __cplusplus
}
#endif
struct CliOptions {
    uint32_t cycles;
    char *tracefile; // Passes NULL if --tf wasn't provided!
    uint8_t size_exponent;
    uint8_t size_mantissa;
    uint8_t round_mode;
    char *input_file;
};

void parse_cli_arguments(int argc, char *argv[], struct CliOptions *options);

struct Request* parse_csv_file(const char *filename, uint32_t *num_requests, uint8_t size_exp, uint8_t size_mant);

uint32_t parse_operand(const char *operand_str, uint8_t size_exp, uint8_t size_mant);

void print_simulation_results(const struct Result *result);

#endif
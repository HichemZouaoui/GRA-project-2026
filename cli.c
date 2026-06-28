#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>  // needed for dirname()
#include "project.h"

static uint8_t parse_and_validate_uint8(const char *str, const char *option_name) {
    char *endptr;
    errno = 0;  // reset before call
    unsigned long val = strtoul(str, &endptr, 10);

    // OLD: val == ULONG_MAX  ← bad, replace with errno
    if (endptr == str || *endptr != '\0' || errno == ERANGE) {
        fprintf(stderr, "Error: Invalid numeric format for %s.\n", option_name);
        exit(1);
    }
    if (val > 255) {
        fprintf(stderr, "Error: Value for %s exceeds 8-bit limit (255).\n", option_name);
        exit(1);
    }
    return (uint8_t)val;
}

void parse_cli_arguments(int argc, char *argv[], struct CliOptions *options) {
    // Set official assignment defaults [cite: 178]
    options->cycles = 1000;          // Default simulation length [cite: 188]
    options->tracefile = NULL;       // NULL indicates no trace logging [cite: 183]
    options->size_exponent = 8;      // Standard IEEE-754 single precision default [cite: 120]
    options->size_mantissa = 23;     // Standard IEEE-754 single precision default [cite: 120]
    options->round_mode = 0;         // Default: Round to nearest, ties to even [cite: 134]
    options->input_file = NULL;

    // Defined structural array utilizing long-only mapping identifiers [cite: 193]
    struct option long_options[] = {
        {"cycles",        required_argument, NULL, 'c'}, //[cite: 180]
        {"tf",            required_argument, NULL, 't'}, //[cite: 183]
        {"size-exponent", required_argument, NULL, 'e'}, //[cite: 157]
        {"size-mantissa", required_argument, NULL, 'm'}, //[cite: 161]
        {"round-mode",    required_argument, NULL, 'r'}, //[cite: 162]
        {"help",          no_argument,       NULL, 'h'}, //[cite: 186]
        {0, 0, 0, 0}
    };

    int opt;
    int long_index = 0;

    // Empty optstring strictly enforces long-only options as designed
    while ((opt = getopt_long(argc, argv, "", long_options, &long_index)) != -1) { //[cite: 193]
        switch (opt) {
            case 'c': {
                char *endptr;
                errno = 0;  // reset before call
                unsigned long val = strtoul(optarg, &endptr, 10);

                // OLD: val > UINT32_MAX  ← still fine as a range check, but add errno too
                if (endptr == optarg || *endptr != '\0' || errno == ERANGE || val > UINT32_MAX) {
                    fprintf(stderr, "Error: Invalid or out-of-bounds value for --cycles.\n");
                    exit(1);
                }
                options->cycles = (uint32_t)val;
                break;
            }
            case 't':
                options->tracefile = optarg;// [cite: 183]
                break;
            case 'e':
                options->size_exponent = parse_and_validate_uint8(optarg, "--size-exponent"); //[cite: 157]
                break;
            case 'm':
                options->size_mantissa = parse_and_validate_uint8(optarg, "--size-mantissa"); //[cite: 161]
                break;
            case 'r':
                options->round_mode = parse_and_validate_uint8(optarg, "--round-mode");// [cite: 162]
                break;
            case 'h':
                printf("Usage: %s --cycles <num> --tf <path> --size-exponent <bits> --size-mantissa <bits> --round-mode <0-4> <file>\n", argv[0]);
                exit(0);
            default:
                fprintf(stderr, "Error: Invalid CLI argument syntax.\n"); //[cite: 191]
                exit(1);
        }
    }

    // Task Validation 1: Round Mode Boundary Enforcement [cite: 131, 190]
    if (options->round_mode > 4) { //[cite: 131]
        fprintf(stderr, "Error: --round-mode must be an integer between 0 and 4.\n"); //[cite: 191]
        exit(1);
    }

    // Task Validation 2: Bit configuration limits (1 sign bit + exponent + mantissa) [cite: 111, 164, 190]
    uint32_t total_bits = (uint32_t)options->size_exponent + (uint32_t)options->size_mantissa + 1;
    if (options->size_exponent == 0 || options->size_mantissa == 0 || total_bits != 32) { //[cite: 164]
        fprintf(stderr, "Error: Custom floating point dimensions must fit inside 32 bits, and parameters cannot be 0.\n"); //[cite: 164, 191]
        exit(1);
    }

    // Tracefile directory writability check
    if (options->tracefile != NULL) {
        // dirname() may modify its argument, so we copy first
        char *tf_copy = strdup(options->tracefile);
        if (!tf_copy) {
            fprintf(stderr, "Error: Memory allocation failure.\n");
            exit(1);
        }

        char *dir = dirname(tf_copy);  // returns "." if no directory component

        if (access(dir, W_OK) != 0) {
            fprintf(stderr, "Error: Tracefile directory '%s' is not writable.\n", dir);
            free(tf_copy);
            exit(1);
        }

        free(tf_copy);
    }

    // Positional Argument Evaluation [cite: 184]
    if (optind < argc) {
        options->input_file = argv[optind];// [cite: 184]

        if (optind + 1 < argc) {
            fprintf(stderr, "Error: Too many positional arguments. Expected exactly one <file>.\n");
            exit(1);
        }
        
        // File system capability test [cite: 190]
        if (access(options->input_file, F_OK) != 0) { //[cite: 190]
            fprintf(stderr, "Error: Target input file '%s' does not exist.\n", options->input_file); //[cite: 191]
            exit(1);
        }
        if (access(options->input_file, R_OK) != 0) { //[cite: 190]
            fprintf(stderr, "Error: Permission denied. Cannot read file '%s'.\n", options->input_file); //[cite: 191]
            exit(1);
        }

        FILE *file = fopen(options->input_file, "r");
        if (!file) {
            fprintf(stderr, "Error: System failed to establish a stream handle for '%s'.\n", options->input_file); //[cite: 191]
            exit(1);
        }

        int first_char = fgetc(file);
        if (first_char == EOF) { //[cite: 234]
            fprintf(stderr, "Error: Target CSV file '%s' is empty.\n", options->input_file); //[cite: 234]
            fclose(file);
            exit(1);
        }
        fclose(file);
    } else {
        fprintf(stderr, "Error: Missing mandatory positional argument <file> specifying requests.\n"); //[cite: 191]
        exit(1);
    }
}
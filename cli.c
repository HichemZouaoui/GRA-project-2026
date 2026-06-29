#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#include "project.h"

static int parse_u32(const char* text, uint32_t* out) {
    if (text == NULL || *text == '\0' || *text == '-') {
        return 0;
    }
    char* end = NULL;
    errno = 0;
    unsigned long value = strtoul(text, &end, 10);
    if (end == text || *end != '\0' || errno == ERANGE || value > UINT32_MAX) {
        return 0;
    }
    *out = (uint32_t)value;
    return 1;
}

static int parse_u8(const char* text, uint8_t* out) {
    uint32_t value = 0;
    if (!parse_u32(text, &value) || value > 255u) {
        return 0;
    }
    *out = (uint8_t)value;
    return 1;
}

static void print_help(const char* program) {
    printf("Usage: %s [options] <input.csv>\n", program);
    printf("Options:\n");
    printf("  --cycles <n>          Number of simulation cycles\n");
    printf("  --tf <path>           Tracefile base/path; .vcd is added if missing\n");
    printf("  --size-exponent <n>   Number of exponent bits\n");
    printf("  --size-mantissa <n>   Number of mantissa bits\n");
    printf("  --round-mode <0-4>    Rounding mode\n");
    printf("  --help                Show this help text\n");
}

void parse_cli_arguments(int argc, char* argv[], struct CliOptions* options) {
    if (options == NULL) {
        fprintf(stderr, "Error: internal null options pointer.\n");
        exit(1);
    }

    options->cycles = 1000;
    options->tracefile = NULL;
    options->size_exponent = 8;
    options->size_mantissa = 23;
    options->round_mode = ROUND_NEAREST_TIES_TO_EVEN;
    options->input_file = NULL;

    static struct option long_options[] = {
        {"cycles",        required_argument, NULL, 'c'},
        {"tf",            required_argument, NULL, 't'},
        {"size-exponent", required_argument, NULL, 'e'},
        {"size-mantissa", required_argument, NULL, 'm'},
        {"round-mode",    required_argument, NULL, 'r'},
        {"help",          no_argument,       NULL, 'h'},
        {0, 0, 0, 0}
    };

    optind = 1;
    opterr = 0;

    int opt = 0;
    while ((opt = getopt_long(argc, argv, "", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
                if (!parse_u32(optarg, &options->cycles)) {
                    fprintf(stderr, "Error: invalid value for --cycles.\n");
                    exit(1);
                }
                break;
            case 't':
                options->tracefile = optarg;
                break;
            case 'e':
                if (!parse_u8(optarg, &options->size_exponent)) {
                    fprintf(stderr, "Error: invalid value for --size-exponent.\n");
                    exit(1);
                }
                break;
            case 'm':
                if (!parse_u8(optarg, &options->size_mantissa)) {
                    fprintf(stderr, "Error: invalid value for --size-mantissa.\n");
                    exit(1);
                }
                break;
            case 'r':
                if (!parse_u8(optarg, &options->round_mode)) {
                    fprintf(stderr, "Error: invalid value for --round-mode.\n");
                    exit(1);
                }
                break;
            case 'h':
                print_help(argv[0]);
                exit(0);
            default:
                fprintf(stderr, "Error: unknown or malformed command-line option.\n");
                exit(1);
        }
    }

    if (options->round_mode > ROUND_TOWARD_NEG_INF) {
        fprintf(stderr, "Error: --round-mode must be between 0 and 4.\n");
        exit(1);
    }

    uint32_t total_bits = 1u + (uint32_t)options->size_exponent + (uint32_t)options->size_mantissa;
    if (options->size_exponent == 0 || options->size_mantissa == 0 || total_bits > 32u) {
        fprintf(stderr, "Error: invalid custom floating-point size.\n");
        exit(1);
    }

    if (options->tracefile != NULL) {
        char* copy = (char*)malloc(strlen(options->tracefile) + 1u);
        if (copy == NULL) {
            fprintf(stderr, "Error: memory allocation failed.\n");
            exit(1);
        }
        strcpy(copy, options->tracefile);
        char* dir = dirname(copy);
        if (dir != NULL && access(dir, W_OK) != 0) {
            fprintf(stderr, "Error: tracefile directory is not writable.\n");
            free(copy);
            exit(1);
        }
        free(copy);
    }

    if (optind >= argc) {
        fprintf(stderr, "Error: missing input CSV file.\n");
        exit(1);
    }
    options->input_file = argv[optind];
    if (optind + 1 < argc) {
        fprintf(stderr, "Error: too many positional input files.\n");
        exit(1);
    }

    if (access(options->input_file, R_OK) != 0) {
        fprintf(stderr, "Error: cannot read input file '%s'.\n", options->input_file);
        exit(1);
    }
}

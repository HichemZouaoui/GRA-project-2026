#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>        // ← ADD: needed for errno in op validation
#include "project.h"

// External declaration referencing your dedicated operand module
//extern uint32_t parse_operand(const char *token, uint8_t size_exp, uint8_t size_mant);

static void trim_trailing_whitespace(char *str) {
    int len = (int)strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t')) {
        str[--len] = '\0';
    }
}

struct Request* parse_csv_file(const char *filename, uint32_t *num_requests,
                               uint8_t size_exp, uint8_t size_mant) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: System failed to establish a stream handle for '%s'.\n", filename);
        exit(1);
    }

    uint32_t capacity = 16;
    *num_requests = 0;

    struct Request *requests = calloc(capacity, sizeof(struct Request));
    if (!requests) {
        fprintf(stderr, "Error: Memory allocation failure.\n");
        fclose(file);
        exit(1);
    }

    int c;
    uint32_t field_idx = 0;
    uint32_t line_counter = 1;
    char token[256];
    uint32_t t_len = 0;
    bool in_leading_space = true;

    // FIX: all four flags at outer scope — they must survive across comma-hits
    bool has_op = false, has_r1 = false, has_r2 = false, has_r3 = false;

    do {
        c = fgetc(file);
        bool is_delimiter = (c == ',' || c == '\n' || c == '\r' || c == EOF);

        if (is_delimiter) {
            token[t_len] = '\0';
            trim_trailing_whitespace(token);

            // FIX: one clean parsing block; has_op set here, not in a duplicate below
            if (field_idx == 0 && t_len > 0) {
                // FIX: proper strtoul validation instead of NULL endptr
                char *endptr;
                errno = 0;
                unsigned long val = strtoul(token, &endptr, 10);
                if (endptr == token || *endptr != '\0' || errno == ERANGE || val > 255) {
                    fprintf(stderr, "Error on line %u: Invalid operation code format '%s'.\n",
                            line_counter, token);
                    free(requests);
                    fclose(file);
                    exit(1);
                }
                requests[*num_requests].op = (uint8_t)val;
                has_op = true;  // ← set here, where it's actually reachable
            } else if (field_idx == 1 && t_len > 0) {
                requests[*num_requests].r1 = parse_operand(token, size_exp, size_mant);
                has_r1 = true;
            } else if (field_idx == 2 && t_len > 0) {
                requests[*num_requests].r2 = parse_operand(token, size_exp, size_mant);
                has_r2 = true;
            } else if (field_idx == 3 && t_len > 0) {
                requests[*num_requests].r3 = parse_operand(token, size_exp, size_mant);
                has_r3 = true;
            }

            if (c == ',') {
                field_idx++;
                if (field_idx > 3 && t_len > 0) {  // allow empty trailing comma but not extra data
                    fprintf(stderr, "Error on line %u: Too many comma-separated fields.\n", line_counter);
                    free(requests); fclose(file); exit(1);
                }
            } else if (c == '\n' || c == '\r' || c == EOF) {

                if (field_idx > 0 || t_len > 0) {

                    // FIX: has_op is now outer-scope and correctly set — this works
                    if (!has_op) {
                        fprintf(stderr, "Error on line %u: Missing mandatory operation code.\n",
                                line_counter);
                        free(requests);
                        fclose(file);
                        exit(1);
                    }

                    uint8_t parsed_op = requests[*num_requests].op;
                    if (parsed_op != 8 && parsed_op != 9 && parsed_op != 10 &&
                        parsed_op != 13 && parsed_op != 14 && parsed_op != 15) {
                        fprintf(stderr, "Error on line %u: Invalid operation code %u.\n",
                                line_counter, parsed_op);
                        free(requests);
                        fclose(file);
                        exit(1);
                    }

                    if (parsed_op == 15) {
                        if (!has_r1 || !has_r2 || !has_r3) {
                            fprintf(stderr, "Error on line %u: FMA (15) requires exactly 3 operands.\n",
                                    line_counter);
                            free(requests);
                            fclose(file);
                            exit(1);
                        }
                    } else {
                        if (!has_r1 || !has_r2) {
                            fprintf(stderr, "Error on line %u: Op %u requires exactly 2 operands.\n",
                                    line_counter, parsed_op);
                            free(requests);
                            fclose(file);
                            exit(1);
                        }
                        if (has_r3) {
                            fprintf(stderr, "Error on line %u: Op %u got unexpected 3rd operand.\n",
                                    line_counter, parsed_op);
                            free(requests);
                            fclose(file);
                            exit(1);
                        }
                    }

                    (*num_requests)++;

                    if (*num_requests >= capacity) {
                        uint32_t old_capacity = capacity;
                        capacity *= 2;
                        struct Request *temp = realloc(requests, capacity * sizeof(struct Request));
                        if (!temp) {
                            fprintf(stderr, "Error: Memory reallocation failed.\n");
                            free(requests);
                            fclose(file);
                            exit(1);
                        }
                        requests = temp;
                        memset(&requests[old_capacity], 0,
                               (capacity - old_capacity) * sizeof(struct Request));
                    }
                }

                if (c == '\n') line_counter++;

                // FIX: resets live here — AFTER validation, not before it
                field_idx = 0;
                has_op  = false;
                has_r1  = false;
                has_r2  = false;
                has_r3  = false;
            }

            t_len = 0;
            in_leading_space = true;

        } else {
            if (in_leading_space && (c == ' ' || c == '\t')) continue;
            in_leading_space = false;

            if (t_len < sizeof(token) - 1) {
                token[t_len++] = (char)c;
            } else {
                fprintf(stderr, "Error on line %u: Token exceeds 255-char buffer.\n", line_counter);
                free(requests);
                fclose(file);
                exit(1);
            }
        }
    } while (c != EOF);

    fclose(file);
    return requests;
}
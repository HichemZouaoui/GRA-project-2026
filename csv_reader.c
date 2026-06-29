#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "project.h"

static char* trim(char* s) {
    while (*s && isspace((unsigned char)*s)) {
        ++s;
    }
    char* end = s + strlen(s);
    while (end > s && isspace((unsigned char)*(end - 1))) {
        --end;
    }
    *end = '\0';
    return s;
}

static int valid_op(uint8_t op) {
    return op == OP_FADD || op == OP_FSUB || op == OP_FMUL ||
           op == OP_FMIN || op == OP_FMAX || op == OP_FMA;
}

static int split_four_fields(char* line, char* fields[4]) {
    int count = 0;
    char* start = line;
    for (char* p = line; ; ++p) {
        if (*p == ',' || *p == '\0') {
            if (count >= 4) {
                return 0;
            }
            char old = *p;
            *p = '\0';
            fields[count++] = trim(start);
            if (old == '\0') {
                break;
            }
            start = p + 1;
        }
    }
    return count == 4;
}

static uint8_t parse_op_or_exit(const char* text, uint32_t line_no) {
    if (text == NULL || *text == '\0') {
        fprintf(stderr, "Error on line %u: missing operation code.\n", line_no);
        exit(1);
    }
    char* end = NULL;
    unsigned long value = strtoul(text, &end, 10);
    if (end == text || *end != '\0' || value > 255u) {
        fprintf(stderr, "Error on line %u: invalid operation code.\n", line_no);
        exit(1);
    }
    uint8_t op = (uint8_t)value;
    if (!valid_op(op)) {
        fprintf(stderr, "Error on line %u: unsupported operation code %u.\n", line_no, op);
        exit(1);
    }
    return op;
}

struct Request* parse_csv_file(const char* filename, uint32_t* num_requests, uint8_t size_exp, uint8_t size_mant) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: cannot open CSV file '%s'.\n", filename);
        exit(1);
    }

    uint32_t capacity = 16;
    uint32_t count = 0;
    struct Request* requests = (struct Request*)calloc(capacity, sizeof(struct Request));
    if (requests == NULL) {
        fprintf(stderr, "Error: memory allocation failed.\n");
        fclose(file);
        exit(1);
    }

    char line[1024];
    uint32_t line_no = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        ++line_no;
        line[strcspn(line, "\r\n")] = '\0';
        char* trimmed = trim(line);
        if (*trimmed == '\0') {
            continue;
        }

        char* fields[4] = {NULL, NULL, NULL, NULL};
        if (!split_four_fields(trimmed, fields)) {
            fprintf(stderr, "Error on line %u: expected exactly four comma-separated fields.\n", line_no);
            free(requests);
            fclose(file);
            exit(1);
        }

        uint8_t op = parse_op_or_exit(fields[0], line_no);
        if (fields[1][0] == '\0' || fields[2][0] == '\0') {
            fprintf(stderr, "Error on line %u: missing mandatory operand.\n", line_no);
            free(requests);
            fclose(file);
            exit(1);
        }
        if (op == OP_FMA && fields[3][0] == '\0') {
            fprintf(stderr, "Error on line %u: FMA requires the third operand.\n", line_no);
            free(requests);
            fclose(file);
            exit(1);
        }

        if (count >= capacity) {
            capacity *= 2u;
            struct Request* grown = (struct Request*)realloc(requests, capacity * sizeof(struct Request));
            if (grown == NULL) {
                fprintf(stderr, "Error: memory reallocation failed.\n");
                free(requests);
                fclose(file);
                exit(1);
            }
            requests = grown;
        }

        requests[count].op = op;
        requests[count].r1 = parse_operand(fields[1], size_exp, size_mant);
        requests[count].r2 = parse_operand(fields[2], size_exp, size_mant);
        requests[count].r3 = fields[3][0] == '\0' ? 0u : parse_operand(fields[3], size_exp, size_mant);
        requests[count].ro = 0u;
        ++count;
    }

    fclose(file);
    *num_requests = count;
    return requests;
}

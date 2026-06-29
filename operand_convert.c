#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "project.h"

static char* trim_in_place(char* s) {
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

static int equals_ignore_case(const char* a, const char* b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return 0;
        }
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static uint32_t mask_bits(uint8_t bits) {
    return bits >= 32 ? 0xFFFFFFFFu : ((1u << bits) - 1u);
}

static uint32_t pack_bits(uint8_t size_exp, uint8_t size_mant, int sign, uint32_t exponent, uint32_t mantissa) {
    uint32_t exp_mask = mask_bits(size_exp);
    uint32_t mant_mask = mask_bits(size_mant);
    uint32_t sign_bit = sign ? (1u << (size_exp + size_mant)) : 0u;
    return sign_bit | ((exponent & exp_mask) << size_mant) | (mantissa & mant_mask);
}

static uint64_t round_nearest_even(long double x) {
    long double fl = floorl(x);
    long double frac = x - fl;
    uint64_t base = (uint64_t)fl;
    const long double eps = 1e-18L;
    if (frac > 0.5L + eps) {
        return base + 1u;
    }
    if (fabsl(frac - 0.5L) <= eps && (base & 1ull)) {
        return base + 1u;
    }
    return base;
}

static uint32_t encode_float_literal(long double value, uint8_t size_exp, uint8_t size_mant) {
    uint32_t max_exp = mask_bits(size_exp);
    uint32_t mant_limit = 1u << size_mant;
    uint32_t bias = (1u << (size_exp - 1u)) - 1u;
    int sign = signbit(value) ? 1 : 0;

    if (isnan(value)) {
        return pack_bits(size_exp, size_mant, 0, max_exp, 1u);
    }
    if (isinf(value)) {
        return pack_bits(size_exp, size_mant, sign, max_exp, 0u);
    }
    if (value == 0.0L) {
        return pack_bits(size_exp, size_mant, sign, 0u, 0u);
    }

    long double av = fabsl(value);
    long double min_norm = ldexpl(1.0L, 1 - (int)bias);
    long double max_norm = ldexpl(2.0L - ldexpl(1.0L, -(int)size_mant), (int)(max_exp - 1u) - (int)bias);

    if (av > max_norm) {
        return pack_bits(size_exp, size_mant, sign, max_exp, 0u);
    }

    if (av < min_norm) {
        long double scaled = av / min_norm * ldexpl(1.0L, size_mant);
        uint64_t mant = round_nearest_even(scaled);
        if (mant == 0) {
            return pack_bits(size_exp, size_mant, sign, 0u, 0u);
        }
        if (mant >= mant_limit) {
            return pack_bits(size_exp, size_mant, sign, 1u, 0u);
        }
        return pack_bits(size_exp, size_mant, sign, 0u, (uint32_t)mant);
    }

    int exp2 = 0;
    long double frac = frexpl(av, &exp2);
    frac *= 2.0L;
    --exp2;
    int biased = exp2 + (int)bias;

    long double scaled = (frac - 1.0L) * ldexpl(1.0L, size_mant);
    uint64_t mant = round_nearest_even(scaled);
    if (mant >= mant_limit) {
        mant = 0;
        ++biased;
    }
    if (biased >= (int)max_exp) {
        return pack_bits(size_exp, size_mant, sign, max_exp, 0u);
    }
    if (biased <= 0) {
        return pack_bits(size_exp, size_mant, sign, 0u, 0u);
    }
    return pack_bits(size_exp, size_mant, sign, (uint32_t)biased, (uint32_t)mant);
}

uint32_t parse_operand(const char* operand_str, uint8_t size_exp, uint8_t size_mant) {
    if (operand_str == NULL) {
        return 0u;
    }

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s", operand_str);
    char* token = trim_in_place(buffer);
    if (*token == '\0') {
        return 0u;
    }

    if (equals_ignore_case(token, "inf") || equals_ignore_case(token, "+inf") ||
        equals_ignore_case(token, "infinity") || equals_ignore_case(token, "+infinity")) {
        return pack_bits(size_exp, size_mant, 0, mask_bits(size_exp), 0u);
    }
    if (equals_ignore_case(token, "-inf") || equals_ignore_case(token, "-infinity")) {
        return pack_bits(size_exp, size_mant, 1, mask_bits(size_exp), 0u);
    }
    if (equals_ignore_case(token, "nan") || equals_ignore_case(token, "+nan") || equals_ignore_case(token, "-nan")) {
        return pack_bits(size_exp, size_mant, 0, mask_bits(size_exp), 1u);
    }

    if (strncmp(token, "0x", 2) == 0 || strncmp(token, "0X", 2) == 0) {
        char* end = NULL;
        errno = 0;
        unsigned long value = strtoul(token, &end, 16);
        if (end == token || *end != '\0' || errno == ERANGE) {
            fprintf(stderr, "Error: malformed hex operand '%s'.\n", token);
            exit(1);
        }
        return (uint32_t)value;
    }

    int looks_float = 0;
    for (const char* p = token; *p; ++p) {
        if (*p == '.' || *p == 'e' || *p == 'E') {
            looks_float = 1;
            break;
        }
    }

    char* end = NULL;
    errno = 0;
    if (!looks_float) {
        if (*token == '-') {
            fprintf(stderr, "Error: negative raw integer operand '%s'.\n", token);
            exit(1);
        }
        unsigned long value = strtoul(token, &end, 10);
        if (end != token && *end == '\0' && errno != ERANGE) {
            return (uint32_t)value;
        }
    }

    errno = 0;
    long double value = strtold(token, &end);
    if (end == token || *end != '\0' || errno == ERANGE) {
        fprintf(stderr, "Error: malformed floating-point operand '%s'.\n", token);
        exit(1);
    }
    return encode_float_literal(value, size_exp, size_mant);
}

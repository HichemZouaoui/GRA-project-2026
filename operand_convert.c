#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

// Translates a standard IEEE-754 float into the custom hardware bit-width
static uint32_t convert_float_to_custom(float f, uint8_t size_exp, uint8_t size_mant) {
    if (f == 0.0f) {
        union { float f_val; uint32_t raw; } u;
        u.f_val = f;
        return u.raw & 0x80000000; // preserves -0 sign
    }

    // Extract raw IEEE-754 components using union type-punning
    union { float f_val; uint32_t raw; } u;
    u.f_val = f;
    
    uint32_t sign = (u.raw >> 31) & 0x1;
    int32_t ieee_exp = ((u.raw >> 23) & 0xFF) - 127;
    uint32_t ieee_mant = u.raw & 0x7FFFFF;

    uint32_t custom_max_exp = (1u << size_exp) - 1;
    
    // Handle IEEE Infinity and NaN bounds
    if (((u.raw >> 23) & 0xFF) == 0xFF) {
        if (ieee_mant != 0) {
            return (sign << 31) | (custom_max_exp << size_mant) | 1; // NaN
        }
        return (sign << 31) | (custom_max_exp << size_mant); // INF
    }

    // Rebias the exponent to the custom width [cite: 119]
    int32_t custom_bias = (int32_t)((1u << (size_exp - 1)) - 1);
    int32_t new_exp = ieee_exp + custom_bias;

    // Handle Hardware Bounds
    if (new_exp >= custom_max_exp) { 
        // Overflow clamps to INF
        return (sign << 31) | (custom_max_exp << size_mant);
    }
    if (new_exp <= 0) { 
        // Underflow drops to 0 (Subnormals bypass )
        return sign << 31; 
    }

    // Align the mantissa through truncation or zero-padding [cite: 118]
    uint32_t custom_mant = 0;
    if (size_mant <= 23) {
        custom_mant = ieee_mant >> (23 - size_mant); 
    } else {
        custom_mant = ieee_mant << (size_mant - 23);
    }

    return (sign << 31) | ((uint32_t)new_exp << size_mant) | custom_mant;
}

// Main parser endpoint
uint32_t parse_operand(const char *token, uint8_t size_exp, uint8_t size_mant) {
    // 1. Handle missing operands (empty string)
    if (token == NULL || *token == '\0') {
        return 0;
    }

    // 2. Identify Hexadecimal strings (e.g., 0x3f800000) [cite: 230]
    if (strncmp(token, "0x", 2) == 0 || strncmp(token, "0X", 2) == 0) {
        char *endptr;
        unsigned long val = strtoul(token, &endptr, 16);
        if (*endptr != '\0') {
            fprintf(stderr, "Error: Malformed hex token '%s'.\n", token);
            exit(1);
        }
        return (uint32_t)val;
    }

    // 3. Identify Float literals by structural markers [cite: 230]
    bool is_float = false;
    for (int i = 0; token[i] != '\0'; i++) {
        if (token[i] == '.' || token[i] == 'e' || token[i] == 'E') {
            is_float = true;
            break;
        }
    }

    if (is_float) {
        errno = 0;
        float f = strtof(token, NULL);
        if(errno == ERANGE){
            fprintf(stderr, "Warning: float '%s' out of range\n", token);
        }
        return convert_float_to_custom(f, size_exp, size_mant); // Repackage float 
    }

    // 4. Fallback: Parse strictly as a raw 32-bit decimal integer [cite: 231]
    // Fallback: Parse strictly as a raw 32-bit decimal integer
    const char *p = token;
    while (*p == ' ' || *p == '\t') p++;  // skip any leading whitespace

    if (*p == '-') {
        fprintf(stderr, "Error: Negative integers are not valid operands: '%s'.\n", token);
        exit(1);
    }
    
    char *endptr;
    errno = 0;  // reset before call
    unsigned long raw_val = strtoul(token, &endptr, 10);

    // OLD: only checked *endptr != '\0'
    if (*endptr != '\0' || errno == ERANGE) {
        fprintf(stderr, "Error: Malformed token '%s'. Not a valid hex, float, or integer.\n", token);
        exit(1);
    }
    
    return (uint32_t)raw_val;
}
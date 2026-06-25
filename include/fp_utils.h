#ifndef FP_UTILS_H
#define FP_UTILS_H

#include <stdint.h>

class FPUtils {
public:
    FPUtils(uint8_t sizeExponent, uint8_t sizeMantissa);

    uint8_t getSizeExponent() const;
    uint8_t getSizeMantissa() const;
    uint8_t getTotalBits() const;
    uint32_t getBias() const;

    uint32_t pack(bool sign, uint32_t exponent, uint32_t mantissa) const;

    bool getSign(uint32_t value) const;
    uint32_t getExponent(uint32_t value) const;
    uint32_t getMantissa(uint32_t value) const;

    uint32_t getPositiveZero() const;
    uint32_t getNegativeZero() const;
    uint32_t getPositiveInf() const;
    uint32_t getNegativeInf() const;
    uint32_t getNaN() const;

    double getMax() const;
    double getMin() const;

    bool isZero(uint32_t value) const;
    bool isInf(uint32_t value) const;
    bool isNaN(uint32_t value) const;
    bool isSubnormal(uint32_t value) const;

    long double decode(uint32_t value) const;

    uint32_t encode(long double value, uint8_t roundMode, bool* inexact = nullptr) const;

private:
    uint8_t sizeExponent;
    uint8_t sizeMantissa;

    uint32_t getExponentMask() const;
    uint32_t getMantissaMask() const;
    uint32_t getMaxExponent() const;
    
};

#endif
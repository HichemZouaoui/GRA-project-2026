#include "fp_utils.h"

#include <cmath>
#include <limits>

static uint64_t roundToUint64(long double value, uint8_t roundMode, bool sign, bool& inexact) {
    long double down = std::floor(value);
    long double rest = value - down;
    inexact = rest != 0.0L;

    if (!inexact) {
        return static_cast<uint64_t>(down);
    }

    switch (roundMode) {
        case 0: {
            if (rest > 0.5L) {
                return static_cast<uint64_t>(down + 1.0L);
            }
            if (rest < 0.5L) {
                return static_cast<uint64_t>(down);
            }
            uint64_t downInt = static_cast<uint64_t>(down);
            if ((downInt % 2U) != 0) {
                return downInt + 1U;
            }
            return downInt;
        }

        case 1:
            if (rest >= 0.5L) {
                return static_cast<uint64_t>(down + 1.0L);
            }
            return static_cast<uint64_t>(down);

        case 2:
            return static_cast<uint64_t>(down);

        case 3:
            if (!sign) {
            return static_cast<uint64_t>(down + 1.0L);
            }
            return static_cast<uint64_t>(down);

        case 4: 
            if (sign) {
            return static_cast<uint64_t>(down + 1.0L);
            }
            return static_cast<uint64_t>(down);

        default:
            return static_cast<uint64_t>(down);

    }

}

FPUtils::FPUtils(uint8_t sizeExponent, uint8_t sizeMantissa) {
    this->sizeExponent = sizeExponent;
    this->sizeMantissa = sizeMantissa;
}

uint8_t FPUtils::getSizeExponent() const {
    return sizeExponent;
}

uint8_t FPUtils::getSizeMantissa() const {
    return sizeMantissa;
}

uint8_t FPUtils::getTotalBits() const {
    return 1 + sizeExponent + sizeMantissa;
}

uint32_t FPUtils::getBias() const {
    return (1U << (sizeExponent - 1)) -1;
}

uint32_t FPUtils::getMaxExponent() const {
    return (1U << sizeExponent) - 1;
}

uint32_t FPUtils::getMantissaMask() const {
    return (1U << sizeMantissa) - 1;
}

uint32_t FPUtils::getExponentMask() const {
    return getMaxExponent();
}

uint32_t FPUtils::pack(bool sign, uint32_t exponent, uint32_t mantissa) const {
    uint32_t signPart = 0;

    if (sign) {
        signPart = 1U << (sizeExponent + sizeMantissa);
    }

    uint32_t exponentPart = (exponent & getExponentMask()) << sizeMantissa;
    uint32_t mantissaPart = mantissa & getMantissaMask();

    return signPart | exponentPart | mantissaPart;
}

bool FPUtils::getSign(uint32_t value) const {
    return ((value >> (sizeExponent + sizeMantissa)) & 1U) != 0;
}

uint32_t FPUtils::getExponent(uint32_t value) const {
    return (value >> sizeMantissa) & getExponentMask();
}

uint32_t FPUtils::getMantissa(uint32_t value) const {
    return value & getMantissaMask();
}

uint32_t FPUtils::getPositiveZero() const {
    return pack(false, 0, 0);
}

uint32_t FPUtils::getNegativeZero() const {
    return pack(true, 0, 0);
}

uint32_t FPUtils::getPositiveInf() const {
    return pack(false, getMaxExponent(), 0);
}

uint32_t FPUtils::getNegativeInf() const {
    return pack(true, getMaxExponent(), 0);
}

uint32_t FPUtils::getNaN() const {
    return pack(false, getMaxExponent(), 1);
}

bool FPUtils::isZero(uint32_t value) const {
    return getExponent(value) == 0 && getMantissa(value) == 0;
}

bool FPUtils::isInf(uint32_t value) const {
    return getExponent(value) == getMaxExponent() && getMantissa(value) == 0;
}

bool FPUtils::isNaN(uint32_t value) const {
    return getExponent(value) == getMaxExponent() && getMantissa(value) != 0;
}

bool FPUtils::isSubnormal(uint32_t value) const {
    return getExponent(value) == 0 && getMantissa(value) != 0;
}

double FPUtils::getMax() const {
    uint32_t value = pack(false, getMaxExponent() - 1U, getMantissaMask());
    return static_cast<double>(decode(value));
}

double FPUtils::getMin() const {
    uint32_t value = pack(false, 1, 0);
    return static_cast<double>(decode(value));
}

long double FPUtils::decode(uint32_t value) const {
    bool sign = getSign(value);
    uint32_t exponent = getExponent(value);
    uint32_t mantissa = getMantissa(value);

    if (isNaN(value)) {
        return std::numeric_limits<long double>::quiet_NaN();
    }
    if (isInf(value)) {
        if (sign) {
            return -std::numeric_limits<long double>::infinity();
        }
        return std::numeric_limits<long double>::infinity();
    }

    if (isZero(value)) {
        if(sign) {
            return -0.0L;
        }
        return 0.0L;
    }

    long double mantissaScale = static_cast<long double>(1ULL << sizeMantissa);
    long double number = 0.0L;

    if (exponent == 0) {
        number = static_cast<long double>(mantissa) / mantissaScale;
        number = std::ldexp(number, 1 - static_cast<int>(getBias()));
    } 
    else{
        number = 1.0L + static_cast<long double>(mantissa) / mantissaScale;
        number = std::ldexp(number, static_cast<int>(exponent) - static_cast<int>(getBias()));
    }

    if (sign) {
        return -number;
    }
    return number;
}

uint32_t FPUtils::encode(long double value, uint8_t roundMode, bool* inexact) const {
    if (inexact != nullptr) {
        *inexact = false;
    }
    if (std::isnan(value)) {
        return getNaN();
    }

    bool sign = std::signbit(value);
    long double absValue = std::fabs(value);

    if (std::isinf(absValue)) {
        if (sign) {
            return getNegativeInf();
        }
        return getPositiveInf();
    }

    if (absValue == 0.0L) {
        if (sign) {
            return getNegativeZero();
        }
        return getPositiveZero();
    }
    long double maxValue = static_cast<long double>(getMax());

    if (absValue > maxValue) {
        if (inexact != nullptr) {
            *inexact = true;
        }
        if (sign) {
            return getNegativeInf();
        }
        return getPositiveInf();
    }

    int exponentValue = 0;
    long double fraction = std::frexp(absValue, &exponentValue);

    long double significand = fraction * 2.0L;
    int realExponent = exponentValue - 1;
    int storedExponent = realExponent + static_cast<int>(getBias());
    bool localInexact = false;

    if (storedExponent <= 0) {
        if (inexact != nullptr) {
            *inexact = true;
        }
        if (sign) {
            return getNegativeZero();
        }
        return getPositiveZero();
    }
    if (storedExponent >= static_cast<int>(getMaxExponent())) {
        if (inexact != nullptr) {
            *inexact = true;
        }
        if (sign) {
            return getNegativeInf();
        }
        return getPositiveInf();
    }

    long double scaledMantissa = (significand - 1.0L) * static_cast<long double>(1ULL << sizeMantissa);
    uint64_t roundedMantissa = roundToUint64(scaledMantissa, roundMode, sign, localInexact);

    if (inexact != nullptr && localInexact) {
        *inexact = true;
    }
    if (roundedMantissa >= (1ULL << sizeMantissa)) {
        roundedMantissa = 0;
        storedExponent++;

        if (storedExponent >= static_cast<int>(getMaxExponent())){
            if (inexact != nullptr) {
                *inexact = true;
            }
            if (sign) {
                return getNegativeInf();
            }
            return getPositiveInf();
        }
    }
    return pack(sign, static_cast<uint32_t>(storedExponent), static_cast<uint32_t>(roundedMantissa));
}
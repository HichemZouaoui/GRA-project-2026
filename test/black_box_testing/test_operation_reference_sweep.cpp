#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

#include "fp_ops.h"
#include "fp_utils.h"
#include "test_common.h"

#define ASSERT_REF_SWEEP(cond, msg) do { if (!(cond)) { std::cerr << "[FAIL] test_operation_reference_sweep: " << msg << " (" << #cond << ")" << std::endl; std::exit(EXIT_FAILURE); } } while (0)

struct RefFormat {
    uint8_t expBits;
    uint8_t mantBits;

    uint32_t expMask() const { return (1u << expBits) - 1u; }
    uint32_t mantMask() const { return (1u << mantBits) - 1u; }
    uint32_t signMask() const { return 1u << (expBits + mantBits); }
    uint32_t bias() const { return (1u << (expBits - 1u)) - 1u; }
    uint32_t pack(bool sign, uint32_t exp, uint32_t mant) const {
        return (sign ? signMask() : 0u) | ((exp & expMask()) << mantBits) | (mant & mantMask());
    }
    bool sign(uint32_t bits) const { return (bits & signMask()) != 0; }
    uint32_t exp(uint32_t bits) const { return (bits >> mantBits) & expMask(); }
    uint32_t mant(uint32_t bits) const { return bits & mantMask(); }
    bool isZero(uint32_t bits) const { return exp(bits) == 0 && mant(bits) == 0; }
    bool isInf(uint32_t bits) const { return exp(bits) == expMask() && mant(bits) == 0; }
    bool isNaN(uint32_t bits) const { return exp(bits) == expMask() && mant(bits) != 0; }
    uint32_t posInf() const { return pack(false, expMask(), 0); }
    uint32_t negInf() const { return pack(true, expMask(), 0); }
    uint32_t nan() const { return pack(false, expMask(), 1); }

    long double decode(uint32_t bits) const {
        if (isNaN(bits)) return std::numeric_limits<long double>::quiet_NaN();
        if (isInf(bits)) return sign(bits) ? -std::numeric_limits<long double>::infinity() : std::numeric_limits<long double>::infinity();
        if (isZero(bits)) return sign(bits) ? -0.0L : 0.0L;
        long double frac = 1.0L + static_cast<long double>(mant(bits)) / std::ldexp(1.0L, mantBits);
        int e = static_cast<int>(exp(bits)) - static_cast<int>(bias());
        long double v = std::ldexp(frac, e);
        return sign(bits) ? -v : v;
    }

    uint32_t encode(long double value, uint8_t mode) const {
        if (std::isnan(value)) return nan();
        bool s = std::signbit(value);
        if (std::isinf(value)) return s ? negInf() : posInf();
        if (value == 0.0L) return pack(s, 0, 0);

        long double av = std::fabs(value);
        long double minNorm = std::ldexp(1.0L, 1 - static_cast<int>(bias()));
        long double maxNorm = std::ldexp(2.0L - std::ldexp(1.0L, -mantBits), static_cast<int>(expMask() - 1u) - static_cast<int>(bias()));
        if (av < minNorm) return pack(s, 0, 0);
        if (av > maxNorm) return s ? negInf() : posInf();

        int exp2 = 0;
        long double frac = std::frexp(av, &exp2);
        frac *= 2.0L;
        exp2 -= 1;
        int biased = exp2 + static_cast<int>(bias());
        long double scaled = (frac - 1.0L) * std::ldexp(1.0L, mantBits);
        uint64_t floorMant = static_cast<uint64_t>(std::floor(scaled));
        long double rest = scaled - std::floor(scaled);
        bool inc = false;
        const long double eps = 1e-18L;
        bool tie = std::fabs(rest - 0.5L) <= eps;
        switch (mode) {
            case ROUND_NEAREST_TIES_TO_EVEN:
                inc = (rest > 0.5L && !tie) || (tie && (floorMant & 1ull));
                break;
            case ROUND_NEAREST_TIES_AWAY_ZERO:
                inc = (rest > 0.5L || tie);
                break;
            case ROUND_TOWARD_ZERO:
                inc = false;
                break;
            case ROUND_TOWARD_POS_INF:
                inc = !s && rest > eps;
                break;
            case ROUND_TOWARD_NEG_INF:
                inc = s && rest > eps;
                break;
        }
        uint64_t mantissa = floorMant + (inc ? 1ull : 0ull);
        if (mantissa >= (1ull << mantBits)) {
            mantissa = 0;
            ++biased;
            if (biased >= static_cast<int>(expMask())) return s ? negInf() : posInf();
        }
        return pack(s, static_cast<uint32_t>(biased), static_cast<uint32_t>(mantissa));
    }
};

static uint32_t refOp(uint8_t op, uint32_t a, uint32_t b, const RefFormat& f, uint8_t mode) {
    if (f.isNaN(a) || f.isNaN(b)) return f.nan();
    if (op == OP_FADD) {
        if (f.isInf(a) || f.isInf(b)) {
            if (f.isInf(a) && f.isInf(b) && f.sign(a) != f.sign(b)) return f.nan();
            return f.isInf(a) ? a : b;
        }
        if (f.isZero(a) && f.isZero(b)) return f.pack(f.sign(a) && f.sign(b), 0, 0);
        return f.encode(f.decode(a) + f.decode(b), mode);
    }
    if (op == OP_FSUB) {
        uint32_t negB = f.pack(!f.sign(b), f.exp(b), f.mant(b));
        return refOp(OP_FADD, a, negB, f, mode);
    }
    if (op == OP_FMUL) {
        if ((f.isZero(a) && f.isNaN(b)) || (f.isZero(b) && f.isNaN(a))) {
            uint32_t z = f.isZero(a) ? a : b;
            return f.pack(f.sign(z), 0, 0);
        }
        if (f.isNaN(a) || f.isNaN(b)) return f.nan();
        if ((f.isInf(a) && f.isZero(b)) || (f.isInf(b) && f.isZero(a))) return f.nan();
        bool rs = f.sign(a) ^ f.sign(b);
        if (f.isInf(a) || f.isInf(b)) return rs ? f.negInf() : f.posInf();
        if (f.isZero(a) || f.isZero(b)) return f.pack(rs, 0, 0);
        return f.encode(f.decode(a) * f.decode(b), mode);
    }
    return 0;
}

int main() {
    std::cout << "--- Starting Operation Reference Sweep Tests ---" << std::endl;

    FPUtils u(8, 23);
    RefFormat f{8, 23};
    std::vector<uint32_t> values = {
        0x00000000u, 0x80000000u,
        0x3F800000u, 0xBF800000u,
        0x40000000u, 0xC0000000u,
        0x40400000u, 0xC0400000u,
        0x3F000000u, 0xBF000000u,
        0x3E800000u, 0xBE800000u,
        0x3DCCCCCDu, 0xBDCCCCCDu,
        0x7F800000u, 0xFF800000u
    };

    for (uint8_t mode = 0; mode <= 4; ++mode) {
        for (uint8_t op : {static_cast<uint8_t>(OP_FADD), static_cast<uint8_t>(OP_FSUB), static_cast<uint8_t>(OP_FMUL)}) {
            for (uint32_t a : values) {
                for (uint32_t b : values) {
                    bool z = false, s = false, o = false, uf = false, ie = false, n = false;
                    uint32_t actual = FPOps::execute(op, a, b, 0, u, mode, z, s, o, uf, ie, n);
                    uint32_t expected = refOp(op, a, b, f, mode);
                    bool bothNaN = f.isNaN(actual) && f.isNaN(expected);
                    if (!(actual == expected || bothNaN)) {
                        std::cerr << std::hex
                                  << "op=0x" << static_cast<unsigned>(op)
                                  << " mode=0x" << static_cast<unsigned>(mode)
                                  << " a=0x" << a
                                  << " b=0x" << b
                                  << " expected=0x" << expected
                                  << " actual=0x" << actual
                                  << std::dec << std::endl;
                    }
                    ASSERT_REF_SWEEP(actual == expected || bothNaN, "operation result does not match independent reference");
                    ASSERT_REF_SWEEP(n == f.isNaN(actual), "NaN flag mismatch against actual result");
                    ASSERT_REF_SWEEP(z == f.isZero(actual), "zero flag mismatch against actual result");
                    ASSERT_REF_SWEEP(s == f.sign(actual), "sign flag mismatch against actual result");
                }
            }
        }
    }

    std::cout << "[SUCCESS] Operation Reference Sweep Tests passed!" << std::endl;
    return 0;
}

#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "fp_ops.h"
#include "fp_utils.h"
#include "fma_core.h"
#include "test_common.h"

#define ASSERT_NAN_CONTRACT(cond, msg) do { if (!(cond)) { std::cerr << "[FAIL] test_nan_payload_and_flags_contract: " << msg << " (" << #cond << ")" << std::endl; std::exit(EXIT_FAILURE); } } while (0)

static bool hasAllOnesExponentAndPayload(uint32_t bits) {
    return (bits & 0x7F800000u) == 0x7F800000u && (bits & 0x007FFFFFu) != 0u;
}

static uint32_t exec(uint8_t op, uint32_t a, uint32_t b, uint32_t c, FPUtils& u,
                     bool& z, bool& s, bool& o, bool& uf, bool& ie, bool& n) {
    z = s = o = uf = ie = n = false;
    return FPOps::execute(op, a, b, c, u, ROUND_NEAREST_TIES_TO_EVEN, z, s, o, uf, ie, n);
}

int main() {
    std::cout << "--- Starting NaN Payload and Flag Contract Tests ---" << std::endl;

    FPUtils u(8, 23);
    bool z, s, o, uf, ie, n;

    const uint32_t qnan1 = 0x7FC00001u;
    const uint32_t qnan2 = 0x7FFFFFFFu;
    const uint32_t snanLike = 0x7FA00001u;
    const uint32_t negNan = 0xFFC12345u;
    const uint32_t one = 0x3F800000u;
    const uint32_t pz = 0x00000000u;
    const uint32_t pi = u.getPositiveInf();
    const uint32_t ni = u.getNegativeInf();

    uint32_t r = exec(OP_FADD, qnan1, one, 0, u, z, s, o, uf, ie, n);
    ASSERT_NAN_CONTRACT(u.isNaN(r) && hasAllOnesExponentAndPayload(r), "FADD qNaN should return a valid NaN payload");
    ASSERT_NAN_CONTRACT(n && !z && !o && !uf, "FADD qNaN flags mismatch");

    r = exec(OP_FSUB, one, snanLike, 0, u, z, s, o, uf, ie, n);
    ASSERT_NAN_CONTRACT(u.isNaN(r) && hasAllOnesExponentAndPayload(r), "FSUB signaling-looking NaN should return valid NaN");
    ASSERT_NAN_CONTRACT(n && !z && !o && !uf, "FSUB signaling-looking NaN flags mismatch");

    r = exec(OP_FMUL, qnan2, one, 0, u, z, s, o, uf, ie, n);
    ASSERT_NAN_CONTRACT(u.isNaN(r) && hasAllOnesExponentAndPayload(r), "FMUL qNaN should return valid NaN");
    ASSERT_NAN_CONTRACT(n && !z && !o && !uf, "FMUL qNaN flags mismatch");

    r = exec(OP_FMIN, negNan, one, 0, u, z, s, o, uf, ie, n);
    ASSERT_NAN_CONTRACT(u.isNaN(r) && hasAllOnesExponentAndPayload(r), "FMIN NaN should return valid NaN");
    ASSERT_NAN_CONTRACT(n && !z && !o && !uf, "FMIN NaN flags mismatch");

    r = exec(OP_FMAX, one, negNan, 0, u, z, s, o, uf, ie, n);
    ASSERT_NAN_CONTRACT(u.isNaN(r) && hasAllOnesExponentAndPayload(r), "FMAX NaN should return valid NaN");
    ASSERT_NAN_CONTRACT(n && !z && !o && !uf, "FMAX NaN flags mismatch");

    r = exec(OP_FADD, pi, ni, 0, u, z, s, o, uf, ie, n);
    ASSERT_NAN_CONTRACT(u.isNaN(r) && hasAllOnesExponentAndPayload(r), "+inf + -inf should produce valid NaN");
    ASSERT_NAN_CONTRACT(n && !z && !o && !uf, "+inf + -inf flags mismatch");

    r = exec(OP_FMUL, pi, pz, 0, u, z, s, o, uf, ie, n);
    ASSERT_NAN_CONTRACT(u.isNaN(r) && hasAllOnesExponentAndPayload(r), "+inf * +0 should produce valid NaN");
    ASSERT_NAN_CONTRACT(n && !z && !o && !uf, "+inf * +0 flags mismatch");

    r = FMACore::executeFMA(qnan1, one, one, u, ROUND_NEAREST_TIES_TO_EVEN, z, s, o, uf, ie, n);
    ASSERT_NAN_CONTRACT(u.isNaN(r) && hasAllOnesExponentAndPayload(r), "FMA NaN addend should produce valid NaN");
    ASSERT_NAN_CONTRACT(n && !z && !o && !uf, "FMA NaN addend flags mismatch");

    r = FMACore::executeFMA(one, pi, pz, u, ROUND_NEAREST_TIES_TO_EVEN, z, s, o, uf, ie, n);
    ASSERT_NAN_CONTRACT(u.isNaN(r) && hasAllOnesExponentAndPayload(r), "FMA inf*zero should produce valid NaN");
    ASSERT_NAN_CONTRACT(n && !z && !o && !uf, "FMA inf*zero flags mismatch");

    std::cout << "[SUCCESS] NaN Payload and Flag Contract Tests passed!" << std::endl;
    return 0;
}

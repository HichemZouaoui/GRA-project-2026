#include <iostream>
#include <cstdlib>
#include <cstdint>
#include "fma_core.h"
#include "fp_utils.h"
#include "test_common.h"
#define ASSERT_FMA(cond, msg) do { if (!(cond)) { std::cerr << "[FAIL] test_fma_core: " << msg << " (" << #cond << ")" << std::endl; std::exit(EXIT_FAILURE); } } while (0)
int main(){
    FPUtils utils(8,23); bool z,s,o,u,ie,n;
    uint32_t res = FMACore::executeFMA(0x3F800000,0x40000000,0x40400000,utils,ROUND_NEAREST_TIES_TO_EVEN,z,s,o,u,ie,n);
    ASSERT_FMA(res == 0x40E00000, "basic FMA 1+(2*3) mismatch");
    ASSERT_FMA(!z && !s && !o && !u && !ie && !n, "wrong flags for exact FMA");
    res = FMACore::executeFMA(0x3F800001,0x3F800001,0x3F800000,utils,ROUND_NEAREST_TIES_TO_EVEN,z,s,o,u,ie,n);
    ASSERT_FMA(res == 0x40000001 && !ie, "exact high-precision FMA mismatch");
    res = FMACore::executeFMA(0x80000000,0x00000000,0x00000000,utils,ROUND_NEAREST_TIES_TO_EVEN,z,s,o,u,ie,n);
    ASSERT_FMA(res == 0x00000000 && z && !s, "signed zero FMA mismatch");
    res = FMACore::executeFMA(0x3F800000,utils.getPositiveInf(),0x00000000,utils,0,z,s,o,u,ie,n);
    ASSERT_FMA(utils.isNaN(res) && n, "INF*0 should produce NaN");
    res = FMACore::executeFMA(utils.pack(false,254,0x7FFFFF),utils.pack(false,254,0x7FFFFF),0x40000000,utils,0,z,s,o,u,ie,n);
    ASSERT_FMA(res == utils.getPositiveInf() && o, "overflow should produce +INF");
    std::cout << "[SUCCESS] FMA Core Tests passed!" << std::endl; return 0; }

#pragma once
#if PLATFORM_SUPPORT_SSE3_INTRIN
#include "Core/Math/VectorMath/VectorMathSSE2.h"

#if PLATFORM_WINDOWS
    #include <pmmintrin.h> // SSE3
#elif PLATFORM_MACOS
    #include <pmmintrin.h> // SSE3
#else
    #error "No valid platform. This code requires SSE3 support on Windows or macOS."
#endif

struct FVectorMathSSE3 : public FVectorMathSSE2
{
    static FORCEINLINE FFloat128 VECTORCALL VectorHorizontalAdd(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return _mm_hadd_ps(VectorA, VectorB);
    }
    
    static FORCEINLINE FFloat128 VECTORCALL VectorDot(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        FFloat128 VectorC = VectorMul(VectorA, VectorB);           // (Ax*Bx), (Ay*By), (Az*Bz), (Aw*Bw)
        FFloat128 VectorD = VectorHorizontalAdd(VectorC, VectorC); // (Ax * Bx) + (Ay * By), (Az * Bz) + (Aw * Bw), ...
        return VectorHorizontalAdd(VectorD, VectorD);              // (Ax * Bx) + (Ay * By) + (Az * Bz) + (Aw * Bw), ...
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorHorizontalSum(FFloat128 Vector) noexcept
    {
        FFloat128 VectorA = VectorHorizontalAdd(Vector, Vector);
        return VectorHorizontalAdd(VectorA, VectorA);
    }
};

#endif

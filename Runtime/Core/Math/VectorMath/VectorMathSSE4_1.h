#pragma once
#if PLATFORM_SUPPORT_SSE4_1_INTRIN
#include "Core/Math/VectorMath/VectorMathSSSE3.h"

#if PLATFORM_WINDOWS
    #include <smmintrin.h> // SSE4.1 and SSE4.2
#elif PLATFORM_MACOS
    #include <smmintrin.h> // SSE4.1 and SSE4.2
#else
    #error "No valid platform. This code requires SSE4.1 support on Windows or macOS."
#endif

struct FVectorMathSSE4_1 : public FVectorMathSSSE3
{
    static FORCEINLINE FFloat128 VECTORCALL VectorDot(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        // Mask 0xFF ensures all components participate in the dot product, and the result is replicated across all components.
        static constexpr int32 Mask = 0xFF;
        return _mm_dp_ps(VectorA, VectorB, Mask);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMulInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return _mm_mullo_epi32(VectorA, VectorB);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMinInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return _mm_min_epi32(VectorA, VectorB);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMaxInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return _mm_max_epi32(VectorA, VectorB);
    }
};

#endif

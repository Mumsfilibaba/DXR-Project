#pragma once
#if PLATFORM_SUPPORT_SSE4_1_INTRIN
#include "Core/Math/VectorMath/PlatformVectorMathSSSE3.h"

#if PLATFORM_WINDOWS
    #include <smmintrin.h> // SSE4.1 and SSE4.2
#elif PLATFORM_MACOS
    #include <smmintrin.h> // SSE4.1 and SSE4.2
#else
    #error "No valid platform. This code requires SSE4.1 support on Windows or macOS."
#endif

struct FPlatformVectorMathSSE4_1 : public FPlatformVectorMathSSSE3
{
    static FORCEINLINE FFloat128 VECTORCALL VectorSelect(FFloat128 Mask, FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        // Uses the sign-bit of each lane in Mask.
        return _mm_blendv_ps(VectorB, VectorA, Mask);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorRound(FFloat128 Vector) noexcept
    {
        return _mm_round_ps(Vector, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorFloor(FFloat128 Vector) noexcept
    {
        return _mm_round_ps(Vector, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorCeil(FFloat128 Vector) noexcept
    {
        return _mm_round_ps(Vector, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorTrunc(FFloat128 Vector) noexcept
    {
        return _mm_round_ps(Vector, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorDot(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        // Mask 0xFF ensures all components participate in the dot product, and the result is replicated across all components.
        static constexpr int32 Mask = 0xFF;
        return _mm_dp_ps(VectorA, VectorB, Mask);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorDot3(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        // Include XYZ (0x07) and replicate result to all lanes (0xF0).
        static constexpr int32 Mask = 0xF7;
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

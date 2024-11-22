#pragma once
#if PLATFORM_SUPPORT_SSE_INTRIN
#include "Core/Math/MathCommon.h"

#if PLATFORM_WINDOWS
    #include <xmmintrin.h>
    #include <immintrin.h>
#elif PLATFORM_MACOS
    #include <immintrin.h>
#else
    #error No valid platform
#endif

typedef __m128  FFloat128;
typedef __m128i FInt128;

struct FVectorMathSSE
{
    static FORCEINLINE FFloat128 VECTORCALL VectorLoad(const float* Source) noexcept
    {
        return _mm_load_ps(Source);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSet(float x, float y, float z, float w) noexcept
    {
        return _mm_set_ps(w, z, y, x);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSet1(float Scalar) noexcept
    {
        return _mm_set_ps1(Scalar);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSetScalar(float Scalar) noexcept
    {
        return _mm_set_ss(Scalar);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorOne() noexcept
    {
        return _mm_set_ps1(1.0f);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorZero() noexcept
    {
        return _mm_setzero_ps();
    }

    static FORCEINLINE FInt128 VECTORCALL VectorLoadInt(const int32* Array) noexcept
    {
        return _mm_load_si128(reinterpret_cast<const __m128i*>(Array));
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSetInt(int32 x, int32 y, int32 z, int32 w) noexcept
    {
        return _mm_set_epi32(w, z, y, x);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSetInt1(int32 Scalar) noexcept
    {
        return _mm_set1_epi32(Scalar);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorIntToFloat(FInt128 Vector) noexcept
    {
        return _mm_castsi128_ps(Vector);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorFloatToInt(FFloat128 Vector) noexcept
    {
        return _mm_castps_si128(Vector);
    }

    static FORCEINLINE void VECTORCALL VectorStore(FFloat128 Vector, float* Dest) noexcept
    {
        return _mm_store_ps(Dest, Vector);
    }

    template<uint8 x, uint8 y, uint8 z, uint8 w>
    static FORCEINLINE FFloat128 VECTORCALL VectorShuffle(FFloat128 VectorA) noexcept
    {
        static constexpr auto ShuffleMask = _MM_SHUFFLE(w, z, y, x);
        
        if constexpr (x == 0 && y == 1 && z == 0 && w == 1)
        {
            return _mm_movelh_ps(VectorA, VectorA);
        }
        else if constexpr (x == 2 && y == 3 && z == 2 && w == 3)
        {
            return _mm_movehl_ps(VectorA, VectorA);
        }
        else if constexpr (x == 0 && y == 0 && z == 1 && w == 1)
        {
            return _mm_unpacklo_ps(VectorA, VectorA);
        }
        else if constexpr (x == 2 && y == 2 && z == 3 && w == 3)
        {
            return _mm_unpackhi_ps(VectorA, VectorA);
        }
        else
        {
            return _mm_shuffle_ps(VectorA, VectorA, ShuffleMask);
        }
    }

    template<uint8 x, uint8 y, uint8 z, uint8 w>
    static FORCEINLINE FFloat128 VECTORCALL VectorShuffle0011(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        static constexpr auto ShuffleMask = _MM_SHUFFLE(w, z, y, x);
        
        if constexpr (x == 0 && y == 1 && z == 0 && w == 1)
        {
            return _mm_movelh_ps(VectorA, VectorB);
        }
        else if constexpr (x == 2 && y == 3 && z == 2 && w == 3)
        {
            return _mm_movehl_ps(VectorB, VectorA);
        }
        else
        {
            return _mm_shuffle_ps(VectorA, VectorB, ShuffleMask);
        }
    }

    template<uint8 x, uint8 y, uint8 z, uint8 w>
    static FORCEINLINE FFloat128 VECTORCALL VectorShuffle0101(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        if constexpr (x == 0 && y == 0 && z == 1 && w == 1)
        {
            return _mm_unpacklo_ps(VectorA, VectorB);
        }
        else if constexpr (x == 2 && y == 2 && z == 3 && w == 3)
        {
            return _mm_unpackhi_ps(VectorA, VectorB);
        }
        else
        {
            FFloat128 VectorC = VectorShuffle0011<x, z, y, w>(VectorA, VectorB);
            return VectorShuffle<0, 2, 1, 3>(VectorC);
        }
    }

    static FORCEINLINE float VECTORCALL VectorGetX(FFloat128 Vector) noexcept
    {
        return _mm_cvtss_f32(Vector);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorMul(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return _mm_mul_ps(VectorA, VectorB);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorDiv(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return _mm_div_ps(VectorA, VectorB);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorAdd(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return _mm_add_ps(VectorA, VectorB);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSub(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return _mm_sub_ps(VectorA, VectorB);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorHorizontalAdd(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        FFloat128 VectorC = VectorShuffle0011<0, 2, 0, 2>(VectorA, VectorB);
        FFloat128 VectorD = VectorShuffle0011<1, 3, 1, 3>(VectorA, VectorB);
        return VectorAdd(VectorC, VectorD);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSqrt(FFloat128 Vector) noexcept
    {
        return _mm_sqrt_ps(Vector);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorRecipSqrt(FFloat128 Vector) noexcept
    {
        return _mm_rsqrt_ps(Vector);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorRecip(FFloat128 Vector) noexcept
    {
        return _mm_rcp_ps(Vector);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorAnd(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return _mm_and_ps(VectorA, VectorB);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorOr(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return _mm_or_ps(VectorA, VectorB);
    }

    static FORCEINLINE bool VECTORCALL VectorEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        const auto Mask = _mm_movemask_ps(_mm_cmpeq_ps(VectorA, VectorB));
        return 0xf == (Mask & 0xf);
    }

    static FORCEINLINE bool VECTORCALL VectorGreaterThan(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        const auto Mask = _mm_movemask_ps(_mm_cmpgt_ps(VectorA, VectorB));
        return 0xf == (Mask & 0xf);
    }

    static FORCEINLINE bool VECTORCALL VectorGreaterThanOrEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        const auto Mask = _mm_movemask_ps(_mm_cmpge_ps(VectorA, VectorB));
        return 0xf == (Mask & 0xf);
    }

    static FORCEINLINE bool VECTORCALL VectorLessThan(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        const auto Mask = _mm_movemask_ps(_mm_cmplt_ps(VectorA, VectorB));
        return 0xf == (Mask & 0xf);
    }

    static FORCEINLINE bool VECTORCALL VectorLessThanOrEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        const auto Mask = _mm_movemask_ps(_mm_cmple_ps(VectorA, VectorB));
        return 0xf == (Mask & 0xf);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorMin(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return _mm_min_ps(VectorA, VectorB);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorMax(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return _mm_max_ps(VectorA, VectorB);
    }
};

#endif

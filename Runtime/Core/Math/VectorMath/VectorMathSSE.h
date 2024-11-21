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
    static FORCEINLINE FFloat128 VECTORCALL LoadAligned(const float* Source) noexcept
    {
        return _mm_load_ps(Source);
    }

    static FORCEINLINE FFloat128 VECTORCALL Load(float x, float y, float z, float w) noexcept
    {
        return _mm_set_ps(w, z, y, x);
    }

    static FORCEINLINE FFloat128 VECTORCALL Load(float Scalar) noexcept
    {
        return _mm_set_ps1(Scalar);
    }

    static FORCEINLINE FFloat128 VECTORCALL LoadSingle(float Scalar) noexcept
    {
        return _mm_set_ss(Scalar);
    }

    static FORCEINLINE FFloat128 VECTORCALL LoadOnes() noexcept
    {
        return _mm_set_ps1(1.0f);
    }

    static FORCEINLINE FFloat128 VECTORCALL LoadZeros() noexcept
    {
        return _mm_setzero_ps();
    }

    static FORCEINLINE FInt128 VECTORCALL LoadAligned(const int32* Array) noexcept
    {
        return _mm_load_si128(reinterpret_cast<const __m128i*>(Array));
    }

    static FORCEINLINE FInt128 VECTORCALL Load(int32 x, int32 y, int32 z, int32 w) noexcept
    {
        return _mm_set_epi32(w, z, y, x);
    }

    static FORCEINLINE FInt128 VECTORCALL Load(int32 Scalar) noexcept
    {
        return _mm_set1_epi32(Scalar);
    }

    static FORCEINLINE FFloat128 VECTORCALL CastIntToFloat(FInt128 A) noexcept
    {
        return _mm_castsi128_ps(A);
    }

    static FORCEINLINE FInt128 VECTORCALL CastFloatToInt(FFloat128 A) noexcept
    {
        return _mm_castps_si128(A);
    }

    static FORCEINLINE void VECTORCALL StoreAligned(FFloat128 Register, float* Array) noexcept
    {
        return _mm_store_ps(Array, Register);
    }

    template<uint8 x, uint8 y, uint8 z, uint8 w>
    static FORCEINLINE FFloat128 VECTORCALL Shuffle(FFloat128 A) noexcept
    {
        static constexpr auto Mask = _MM_SHUFFLE(w, z, y, x);
        
        if constexpr (x == 0 && y == 1 && z == 0 && w == 1)
        {
            return _mm_movelh_ps(A, A);
        }
        else if constexpr (x == 2 && y == 3 && z == 2 && w == 3)
        {
            return _mm_movehl_ps(A, A);
        }
        else if constexpr (x == 0 && y == 0 && z == 1 && w == 1)
        {
            return _mm_unpacklo_ps(A, A);
        }
        else if constexpr (x == 2 && y == 2 && z == 3 && w == 3)
        {
            return _mm_unpackhi_ps(A, A);
        }
        else
        {
            return _mm_shuffle_ps(A, A, Mask);
        }
    }

    template<uint8 x, uint8 y, uint8 z, uint8 w>
    static FORCEINLINE FFloat128 VECTORCALL Shuffle0011(FFloat128 A, FFloat128 B) noexcept
    {
        static constexpr auto Mask = _MM_SHUFFLE(w, z, y, x);
        
        if constexpr (x == 0 && y == 1 && z == 0 && w == 1)
        {
            return _mm_movelh_ps(A, B);
        }
        else if constexpr (x == 2 && y == 3 && z == 2 && w == 3)
        {
            return _mm_movehl_ps(B, A);
        }
        else
        {
            return _mm_shuffle_ps(A, B, Mask);
        }
    }

    template<uint8 x, uint8 y, uint8 z, uint8 w>
    static FORCEINLINE FFloat128 VECTORCALL Shuffle0101(FFloat128 A, FFloat128 B) noexcept
    {
        if constexpr (x == 0 && y == 0 && z == 1 && w == 1)
        {
            return _mm_unpacklo_ps(A, B);
        }
        else if constexpr (x == 2 && y == 2 && z == 3 && w == 3)
        {
            return _mm_unpackhi_ps(A, B);
        }
        else
        {
            FFloat128 TempA = Shuffle0011<x, z, y, w>(A, B);
            return Shuffle<0, 2, 1, 3>(TempA);
        }
    }

    static FORCEINLINE float VECTORCALL GetX(FFloat128 Register) noexcept
    {
        return _mm_cvtss_f32(Register);
    }

    static FORCEINLINE FFloat128 VECTORCALL Mul(FFloat128 A, FFloat128 B) noexcept
    {
        return _mm_mul_ps(A, B);
    }

    static FORCEINLINE FFloat128 VECTORCALL Div(FFloat128 A, FFloat128 B) noexcept
    {
        return _mm_div_ps(A, B);
    }

    static FORCEINLINE FFloat128 VECTORCALL Add(FFloat128 A, FFloat128 B) noexcept
    {
        return _mm_add_ps(A, B);
    }

    static FORCEINLINE FFloat128 VECTORCALL HorizontalAdd(FFloat128 A, FFloat128 B) noexcept
    {
        FFloat128 Temp0 = Shuffle0011<0, 2, 0, 2>(A, B);
        FFloat128 Temp1 = Shuffle0011<1, 3, 1, 3>(A, B);
        return Add(Temp0, Temp1);
    }

    static FORCEINLINE FFloat128 VECTORCALL Sub(FFloat128 A, FFloat128 B) noexcept
    {
        return _mm_sub_ps(A, B);
    }

    static FORCEINLINE FFloat128 VECTORCALL Sqrt(FFloat128 A) noexcept
    {
        return _mm_sqrt_ps(A);
    }

    static FORCEINLINE FFloat128 VECTORCALL RecipSqrt(FFloat128 A) noexcept
    {
        return _mm_rsqrt_ps(A);
    }

    static FORCEINLINE FFloat128 VECTORCALL Recip(FFloat128 A) noexcept
    {
        return _mm_rcp_ps(A);
    }

    static FORCEINLINE FFloat128 VECTORCALL And(FFloat128 A, FFloat128 B) noexcept
    {
        return _mm_and_ps(A, B);
    }

    static FORCEINLINE FFloat128 VECTORCALL Or(FFloat128 A, FFloat128 B) noexcept
    {
        return _mm_or_ps(A, B);
    }

    static FORCEINLINE bool VECTORCALL Equal(FFloat128 A, FFloat128 B) noexcept
    {
        const auto Mask = _mm_movemask_ps(_mm_cmpeq_ps(A, B));
        return 0xf == (Mask & 0xf);
    }

    static FORCEINLINE bool VECTORCALL GreaterThan(FFloat128 A, FFloat128 B) noexcept
    {
        const auto Mask = _mm_movemask_ps(_mm_cmpgt_ps(A, B));
        return 0xf == (Mask & 0xf);
    }

    static FORCEINLINE bool VECTORCALL GreaterThanOrEqual(FFloat128 A, FFloat128 B) noexcept
    {
        const auto Mask = _mm_movemask_ps(_mm_cmpge_ps(A, B));
        return 0xf == (Mask & 0xf);
    }

    static FORCEINLINE bool VECTORCALL LessThan(FFloat128 A, FFloat128 B) noexcept
    {
        const auto Mask = _mm_movemask_ps(_mm_cmplt_ps(A, B));
        return 0xf == (Mask & 0xf);
    }

    static FORCEINLINE bool VECTORCALL LessThanOrEqual(FFloat128 A, FFloat128 B) noexcept
    {
        const auto Mask = _mm_movemask_ps(_mm_cmple_ps(A, B));
        return 0xf == (Mask & 0xf);
    }

    static FORCEINLINE FFloat128 VECTORCALL Min(FFloat128 A, FFloat128 B) noexcept
    {
        return _mm_min_ps(A, B);
    }

    static FORCEINLINE FFloat128 VECTORCALL Max(FFloat128 A, FFloat128 B) noexcept
    {
        return _mm_max_ps(A, B);
    }
};

#endif

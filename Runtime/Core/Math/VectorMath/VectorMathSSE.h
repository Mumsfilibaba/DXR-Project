#pragma once
#if PLATFORM_SUPPORT_SSE_INTRIN
#include "Core/Math/MathCommon.h"

#if PLATFORM_WINDOWS
    #include <xmmintrin.h> // SSE
#elif PLATFORM_MACOS
    #include <xmmintrin.h> // SSE
#else
    #error "No valid platform. This code requires SSE support on Windows or macOS."
#endif

typedef __m128 FFloat128;
// TODO: define a fallback to typedef __m128i FInt128;

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

    static FORCEINLINE void VECTORCALL VectorStore(FFloat128 Vector, float* Dest) noexcept
    {
        _mm_store_ps(Dest, Vector);
    }

    static FORCEINLINE void VECTORCALL VectorStore3(FFloat128 Vector, float* Dest) noexcept
    {
        ALIGN_AS(16) float Array[4];
        _mm_store_ps(Array, Vector);

        Dest[0] = Array[0];
        Dest[1] = Array[1];
        Dest[2] = Array[2];
    }

    template<uint8 ComponentIndexX, uint8 ComponentIndexY, uint8 ComponentIndexZ, uint8 ComponentIndexW>
    static FORCEINLINE FFloat128 VECTORCALL VectorShuffle(FFloat128 VectorA) noexcept
    {
        static_assert(ComponentIndexX < 4, "ComponentIndexX out of range");
        static_assert(ComponentIndexY < 4, "ComponentIndexY out of range");
        static_assert(ComponentIndexZ < 4, "ComponentIndexZ out of range");
        static_assert(ComponentIndexW < 4, "ComponentIndexW out of range");

        static constexpr auto ShuffleMask = _MM_SHUFFLE(ComponentIndexW, ComponentIndexZ, ComponentIndexY, ComponentIndexX);

        if constexpr (ComponentIndexX == 0 && ComponentIndexY == 1 && ComponentIndexZ == 0 && ComponentIndexW == 1)
        {
            return _mm_movelh_ps(VectorA, VectorA);
        }
        else if constexpr (ComponentIndexX == 2 && ComponentIndexY == 3 && ComponentIndexZ == 2 && ComponentIndexW == 3)
        {
            return _mm_movehl_ps(VectorA, VectorA);
        }
        else if constexpr (ComponentIndexX == 0 && ComponentIndexY == 0 && ComponentIndexZ == 1 && ComponentIndexW == 1)
        {
            return _mm_unpacklo_ps(VectorA, VectorA);
        }
        else if constexpr (ComponentIndexX == 2 && ComponentIndexY == 2 && ComponentIndexZ == 3 && ComponentIndexW == 3)
        {
            return _mm_unpackhi_ps(VectorA, VectorA);
        }
        else
        {
            return _mm_shuffle_ps(VectorA, VectorA, ShuffleMask);
        }
    }

    template<uint8 ComponentIndexX, uint8 ComponentIndexY, uint8 ComponentIndexZ, uint8 ComponentIndexW>
    static FORCEINLINE FFloat128 VECTORCALL VectorShuffle0011(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        static_assert(ComponentIndexX < 4, "ComponentIndexX out of range");
        static_assert(ComponentIndexY < 4, "ComponentIndexY out of range");
        static_assert(ComponentIndexZ < 4, "ComponentIndexZ out of range");
        static_assert(ComponentIndexW < 4, "ComponentIndexW out of range");

        static constexpr auto ShuffleMask = _MM_SHUFFLE(ComponentIndexW, ComponentIndexZ, ComponentIndexY, ComponentIndexX);

        if constexpr (ComponentIndexX == 0 && ComponentIndexY == 1 && ComponentIndexZ == 0 && ComponentIndexW == 1)
        {
            return _mm_movelh_ps(VectorA, VectorB);
        }
        else if constexpr (ComponentIndexX == 2 && ComponentIndexY == 3 && ComponentIndexZ == 2 && ComponentIndexW == 3)
        {
            return _mm_movehl_ps(VectorB, VectorA);
        }
        else
        {
            return _mm_shuffle_ps(VectorA, VectorB, ShuffleMask);
        }
    }

    template<uint8 ComponentIndexX, uint8 ComponentIndexY, uint8 ComponentIndexZ, uint8 ComponentIndexW>
    static FORCEINLINE FFloat128 VECTORCALL VectorShuffle0101(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        static_assert(ComponentIndexX < 4, "ComponentIndexX out of range");
        static_assert(ComponentIndexY < 4, "ComponentIndexY out of range");
        static_assert(ComponentIndexZ < 4, "ComponentIndexZ out of range");
        static_assert(ComponentIndexW < 4, "ComponentIndexW out of range");

        if constexpr (ComponentIndexX == 0 && ComponentIndexY == 0 && ComponentIndexZ == 1 && ComponentIndexW == 1)
        {
            return _mm_unpacklo_ps(VectorA, VectorB);
        }
        else if constexpr (ComponentIndexX == 2 && ComponentIndexY == 2 && ComponentIndexZ == 3 && ComponentIndexW == 3)
        {
            return _mm_unpackhi_ps(VectorA, VectorB);
        }
        else
        {
            FFloat128 VectorC = VectorShuffle0011<ComponentIndexX, ComponentIndexZ, ComponentIndexY, ComponentIndexW>(VectorA, VectorB);
            return VectorShuffle<0, 2, 1, 3>(VectorC);
        }
    }

    template<uint8 ComponentIndex>
    static FORCEINLINE FFloat128 VECTORCALL VectorBroadcast(FFloat128 Vector) noexcept
    {
        static_assert(ComponentIndex < 4, "ComponentIndex out of range");
        return VectorShuffle<ComponentIndex, ComponentIndex, ComponentIndex, ComponentIndex>(Vector);
    }

    static FORCEINLINE float VECTORCALL VectorGetX(FFloat128 Vector) noexcept
    {
        return _mm_cvtss_f32(Vector);
    }

    static FORCEINLINE float VECTORCALL VectorGetY(FFloat128 Vector) noexcept
    {
        FFloat128 ShuffledVector = VectorBroadcast<1>(Vector);
        return _mm_cvtss_f32(ShuffledVector);
    }

    static FORCEINLINE float VECTORCALL VectorGetZ(FFloat128 Vector) noexcept
    {
        FFloat128 ShuffledVector = VectorBroadcast<2>(Vector);
        return _mm_cvtss_f32(ShuffledVector);
    }

    static FORCEINLINE float VECTORCALL VectorGetW(FFloat128 Vector) noexcept
    {
        FFloat128 ShuffledVector = VectorBroadcast<3>(Vector);
        return _mm_cvtss_f32(ShuffledVector);
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

    static FORCEINLINE FFloat128 VECTORCALL VectorHorizontalSum(FFloat128 Vector) noexcept
    {
        FFloat128 ShuffledVector = VectorShuffle<1, 0, 3, 2>(Vector);
        FFloat128 VectorSum = VectorAdd(Vector, ShuffledVector);

        ShuffledVector = VectorShuffle0011<2, 3, 2, 3>(ShuffledVector, VectorSum);
        VectorSum = VectorAdd(ShuffledVector, VectorSum);

        return VectorBroadcast<0>(VectorSum);
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

    static FORCEINLINE FFloat128 VECTORCALL VectorDot(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        FFloat128 VectorC = VectorMul(VectorA, VectorB);        // (Ax*Bx), (Ay*By), (Az*Bz), (Aw*Bw)
        FFloat128 VectorD = VectorShuffle<1, 0, 3, 2>(VectorC); // (Ay*By), (Ax*Bx), (Aw*Bw), (Az*Bz)

        VectorC = VectorAdd(VectorC, VectorD);                  // (Ax * Bx) + (Ay * By), (Ay * By) + (Ax * Bx), (Az * Bz) + (Aw * Bw), (Aw * Bw) + (Az * Bz)
        VectorD = VectorShuffle<2, 3, 0, 1>(VectorC);           // (Az * Bz) + (Aw * Bw), (Aw * Bw) + (Az * Bz), (Ax * Bx) + (Ay * By), (Ay * By) + (Ax * Bx)

        return VectorAdd(VectorC, VectorD);                     // (Ax * Bx) + (Ay * By) + (Az * Bz) + (Aw * Bw), ...
    }

    static FORCEINLINE bool VECTORCALL VectorAllEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        int32 Mask = _mm_movemask_ps(_mm_cmpeq_ps(VectorA, VectorB));
        return 0xf == (Mask & 0xf);
    }

    static FORCEINLINE bool VECTORCALL VectorAllGreaterThan(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        int32 Mask = _mm_movemask_ps(_mm_cmpgt_ps(VectorA, VectorB));
        return 0xf == (Mask & 0xf);
    }

    static FORCEINLINE bool VECTORCALL VectorAllGreaterThanOrEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        int32 Mask = _mm_movemask_ps(_mm_cmpge_ps(VectorA, VectorB));
        return 0xf == (Mask & 0xf);
    }

    static FORCEINLINE bool VECTORCALL VectorAllLessThan(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        int32 Mask = _mm_movemask_ps(_mm_cmplt_ps(VectorA, VectorB));
        return 0xf == (Mask & 0xf);
    }

    static FORCEINLINE bool VECTORCALL VectorAllLessThanOrEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        int32 Mask = _mm_movemask_ps(_mm_cmple_ps(VectorA, VectorB));
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

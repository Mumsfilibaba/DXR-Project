#pragma once
#if PLATFORM_SUPPORT_SSE2_INTRIN
#include "Core/Math/VectorMath/VectorMathSSE.h"

#if PLATFORM_WINDOWS
    #include <emmintrin.h> // SSE2
#elif PLATFORM_MACOS
    #include <emmintrin.h> // SSE2
#else
    #error "No valid platform. This code requires SSE2 support on Windows or macOS."
#endif

typedef __m128i FInt128;

struct FVectorMathSSE2 : public FVectorMathSSE
{
    static FORCEINLINE FInt128 VECTORCALL VectorLoadInt(const int32* Source) noexcept
    {
        return _mm_load_si128(reinterpret_cast<const __m128i*>(Source));
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSetInt(int32 x, int32 y, int32 z, int32 w) noexcept
    {
        return _mm_set_epi32(w, z, y, x);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSetInt1(int32 Scalar) noexcept
    {
        return _mm_set1_epi32(Scalar);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSetScalarInt(int32 Scalar) noexcept
    {
        return _mm_cvtsi32_si128(Scalar);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorZeroInt() noexcept
    {
        return _mm_setzero_si128();
    }

    static FORCEINLINE FInt128 VECTORCALL VectorOneInt() noexcept
    {
        return _mm_set1_epi32(1);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorIntToFloat(FInt128 Vector) noexcept
    {
        return _mm_castsi128_ps(Vector);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorFloatToInt(FFloat128 Vector) noexcept
    {
        return _mm_castps_si128(Vector);
    }

    static FORCEINLINE void VECTORCALL VectorStoreInt(FInt128 Vector, int32* Dest) noexcept
    {
        _mm_store_si128(reinterpret_cast<__m128i*>(Dest), Vector);
    }

    static FORCEINLINE void VECTORCALL VectorStoreInt3(FInt128 Vector, int32* Dest) noexcept
    {
        ALIGN_AS(16) int32 Array[4];
        _mm_store_si128(reinterpret_cast<__m128i*>(Array), Vector);

        Dest[0] = Array[0];
        Dest[1] = Array[1];
        Dest[2] = Array[2];
    }

    static FORCEINLINE FInt128 VECTORCALL VectorAddInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return _mm_add_epi32(VectorA, VectorB);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSubInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return _mm_sub_epi32(VectorA, VectorB);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMulInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // Unpack and multiply individual elements
        FInt128 VectorA_Low  = _mm_unpacklo_epi32(VectorA, _mm_setzero_si128());
        FInt128 VectorB_Low  = _mm_unpacklo_epi32(VectorB, _mm_setzero_si128());
        FInt128 VectorA_High = _mm_unpackhi_epi32(VectorA, _mm_setzero_si128());
        FInt128 VectorB_High = _mm_unpackhi_epi32(VectorB, _mm_setzero_si128());

        FInt128 Prod_Low  = _mm_mul_epu32(VectorA_Low, VectorB_Low);   // Multiply lower two 32-bit integers
        FInt128 Prod_High = _mm_mul_epu32(VectorA_High, VectorB_High); // Multiply higher two 32-bit integers

        // Pack the results back into a single FInt128
        FInt128 Shuffle_Low  = _mm_shuffle_epi32(Prod_Low, _MM_SHUFFLE(0, 0, 0, 0));
        FInt128 Shuffle_High = _mm_shuffle_epi32(Prod_High, _MM_SHUFFLE(2, 2, 2, 2));
        return _mm_or_si128(Shuffle_Low, Shuffle_High);
    }

    static FORCEINLINE bool VECTORCALL VectorEqualInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // Compare for equality
        FInt128 Compare_128 = _mm_cmpeq_epi32(VectorA, VectorB);

        // Reinterpret the comparison result as __m128 to use _mm_movemask_ps
        int32 Mask = _mm_movemask_ps(*reinterpret_cast<const __m128*>(&Compare_128));

        // Check if all four comparison results are true
        return (Mask & 0xf) == 0xf;
    }

    static FORCEINLINE bool VECTORCALL VectorGreaterThanInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // Compare for greater than
        FInt128 Compare_128 = _mm_cmpgt_epi32(VectorA, VectorB);

        // Reinterpret the comparison result as __m128 to use _mm_movemask_ps
        int32 Mask = _mm_movemask_ps(*reinterpret_cast<const __m128*>(&Compare_128));

        // Check if all four comparison results are true
        return (Mask & 0xf) == 0xf;
    }

    static FORCEINLINE bool VECTORCALL VectorGreaterThanOrEqualInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // Check for not less than
        // VectorA >= VectorB is equivalent to !(VectorA < VectorB)
        FInt128 Compare_128 = _mm_cmpgt_epi32(VectorB, VectorA); // VectorB > VectorA

        // Reinterpret as __m128 and extract mask
        int32 Mask = _mm_movemask_ps(*reinterpret_cast<const __m128*>(&Compare_128));

        // All elements should NOT be less than, i.e., no elements should have VectorB > VectorA
        return (Mask & 0xf) == 0x0;
    }

    static FORCEINLINE bool VECTORCALL VectorLessThanInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // Compare for less than by swapping operands
        FInt128 Compare_128 = _mm_cmpgt_epi32(VectorB, VectorA); // VectorB > VectorA

        // Reinterpret the comparison result as __m128 to use _mm_movemask_ps
        int32 Mask = _mm_movemask_ps(*reinterpret_cast<const __m128*>(&Compare_128));

        // Check if all four comparison results are true
        return (Mask & 0xf) == 0xf;
    }

    static FORCEINLINE bool VECTORCALL VectorLessThanOrEqualInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // Check for not greater than
        // VectorA <= VectorB is equivalent to !(VectorA > VectorB)
        FInt128 Compare_128 = _mm_cmpgt_epi32(VectorA, VectorB); // VectorA > VectorB

        // Reinterpret as __m128 and extract mask
        int32 Mask = _mm_movemask_ps(*reinterpret_cast<const __m128*>(&Compare_128));

        // All elements should NOT be greater than, i.e., no elements should have VectorA > VectorB
        return (Mask & 0xf) == 0x0;
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMinInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // Compute (VectorA - VectorB)
        FInt128 Diff = _mm_sub_epi32(VectorA, VectorB);
        
        // Compute sign mask: if A < B, the sign bit of (A - B) will be set
        FInt128 SignMask = _mm_srai_epi32(Diff, 31);  // Arithmetic shift right to get sign bits
        
        // Compute VectorB + ((VectorA - VectorB) & signMask)
        // If A < B, signMask is all 1s, so (A - B) & signMask = A - B
        // Therefore, B + (A - B) = A
        // If A >= B, signMask is 0, so B + 0 = B
        return _mm_add_epi32(VectorB, _mm_and_si128(Diff, SignMask));
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMaxInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // Compute (VectorA - VectorB)
        FInt128 Diff = _mm_sub_epi32(VectorA, VectorB);
        
        // Compute sign mask: if A < B, the sign bit of (A - B) will be set
        FInt128 SignMask = _mm_srai_epi32(Diff, 31);  // Arithmetic shift right to get sign bits
        
        // Compute VectorA - ((VectorA - VectorB) & signMask)
        // If A < B, signMask is all 1s, so (A - B) & signMask = A - B
        // Therefore, A - (A - B) = B
        // If A >= B, signMask is 0, so A - 0 = A
        return _mm_sub_epi32(VectorA, _mm_and_si128(Diff, SignMask));
    }
};

#endif

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
        return _mm_loadu_si128(reinterpret_cast<const __m128i*>(Source));
    }

    static FORCEINLINE FInt128 VECTORCALL VectorLoadIntAligned(const int32* Source) noexcept
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

    // ---------------------------------------------------------------------------------------------
    // Bitwise int ops
    // ---------------------------------------------------------------------------------------------

    static FORCEINLINE FInt128 VECTORCALL VectorAndInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return _mm_and_si128(VectorA, VectorB);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorOrInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return _mm_or_si128(VectorA, VectorB);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorXorInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return _mm_xor_si128(VectorA, VectorB);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorAndNotInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // (~A) & B
        return _mm_andnot_si128(VectorA, VectorB);
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSelectInt(FInt128 Mask, FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // (Mask & A) | (~Mask & B)
        const FInt128 MaskedA = VectorAndInt(Mask, VectorA);
        const FInt128 MaskedB = VectorAndNotInt(Mask, VectorB);
        return VectorOrInt(MaskedA, MaskedB);
    }

    static FORCEINLINE void VECTORCALL VectorStoreInt(FInt128 Vector, int32* Dest) noexcept
    {
        _mm_storeu_si128(reinterpret_cast<__m128i*>(Dest), Vector);
    }

    static FORCEINLINE void VECTORCALL VectorStoreIntAligned(FInt128 Vector, int32* Dest) noexcept
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
        // SSE2 has no 4-wide 32-bit multiply, so we combine two mul_epu32 results.
        // Returns the low 32 bits of each 32-bit product (matches _mm_mullo_epi32 semantics).
        const FInt128 Even64 = _mm_mul_epu32(VectorA, VectorB); // lanes 0 and 2
        const FInt128 A_Odd  = _mm_srli_si128(VectorA, 4);
        const FInt128 B_Odd  = _mm_srli_si128(VectorB, 4);
        const FInt128 Odd64  = _mm_mul_epu32(A_Odd, B_Odd);     // lanes 1 and 3

        const FInt128 EvenLo32 = _mm_shuffle_epi32(Even64, _MM_SHUFFLE(2, 0, 2, 0)); // (p0, p2, p0, p2)
        const FInt128 OddLo32  = _mm_shuffle_epi32(Odd64,  _MM_SHUFFLE(2, 0, 2, 0)); // (p1, p3, p1, p3)

        return _mm_unpacklo_epi32(EvenLo32, OddLo32); // (p0, p1, p2, p3)
    }

    static FORCEINLINE bool VECTORCALL VectorEqualInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        const FInt128 Compare_128 = _mm_cmpeq_epi32(VectorA, VectorB);
        const int32 Mask = _mm_movemask_epi8(Compare_128);
        return Mask == 0xFFFF;
    }

    static FORCEINLINE bool VECTORCALL VectorGreaterThanInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        const FInt128 Compare_128 = _mm_cmpgt_epi32(VectorA, VectorB);
        const int32 Mask = _mm_movemask_epi8(Compare_128);
        return Mask == 0xFFFF;
    }

    static FORCEINLINE bool VECTORCALL VectorGreaterThanOrEqualInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // A >= B <=> !(B > A)
        const FInt128 Compare_128 = _mm_cmpgt_epi32(VectorB, VectorA);
        const int32 Mask = _mm_movemask_epi8(Compare_128);
        return Mask == 0x0000;
    }

    static FORCEINLINE bool VECTORCALL VectorLessThanInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        const FInt128 Compare_128 = _mm_cmpgt_epi32(VectorB, VectorA);
        const int32 Mask = _mm_movemask_epi8(Compare_128);
        return Mask == 0xFFFF;
    }

    static FORCEINLINE bool VECTORCALL VectorLessThanOrEqualInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // A <= B <=> !(A > B)
        const FInt128 Compare_128 = _mm_cmpgt_epi32(VectorA, VectorB);
        const int32 Mask = _mm_movemask_epi8(Compare_128);
        return Mask == 0x0000;
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMinInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        // Min without relying on (A-B) sign (avoids overflow corner cases).
        const FInt128 MaskAgtB = _mm_cmpgt_epi32(VectorA, VectorB); // A > B
        // Select B where (A > B), else A.
        return _mm_or_si128(_mm_and_si128(MaskAgtB, VectorB), _mm_andnot_si128(MaskAgtB, VectorA));
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMaxInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        const FInt128 MaskAgtB = _mm_cmpgt_epi32(VectorA, VectorB); // A > B
        // Select A where (A > B), else B.
        return _mm_or_si128(_mm_and_si128(MaskAgtB, VectorA), _mm_andnot_si128(MaskAgtB, VectorB));
    }
};

#endif

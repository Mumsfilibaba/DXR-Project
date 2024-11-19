#pragma once
#include "Core/Math/MathCommon.h"

#ifndef VECTOR_ALIGN
    #define VECTOR_ALIGN ALIGN_AS(16)
#endif

#if (!defined(DISABLE_SIMD) && PLATFORM_ARCHITECTURE_X86_X64)
    #define USE_VECTOR_OP (1)

#if PLATFORM_WINDOWS
    #include <xmmintrin.h>
    #include <immintrin.h>
#elif PLATFORM_MACOS
    #include <immintrin.h>
#else
    #error No valid platform
#endif

namespace NVectorOp
{
    /* TODO: Add fallback when we have no SSE intrinsics */

#if ENABLE_SSE_INTRIN
    typedef __m128  Float128;
    typedef __m128i Int128;

    FORCEINLINE Float128 VECTORCALL LoadAligned(const float* Array) noexcept
    {
        return _mm_load_ps(Array);
    }

    FORCEINLINE Float128 VECTORCALL Load(float x, float y, float z, float w) noexcept
    {
        return _mm_set_ps(w, z, y, x);
    }

    FORCEINLINE Float128 VECTORCALL Load(float Scalar) noexcept
    {
        return _mm_set_ps1(Scalar);
    }

    FORCEINLINE Float128 VECTORCALL LoadSingle(float Single) noexcept
    {
        return _mm_set_ss(Single);
    }

    FORCEINLINE Float128 VECTORCALL MakeOnes() noexcept
    {
        return _mm_set_ps1(1.0f);
    }

    FORCEINLINE Float128 VECTORCALL MakeZeros() noexcept
    {
        return _mm_setzero_ps();
    }

    FORCEINLINE Int128 VECTORCALL LoadAligned(const int* Array) noexcept
    {
        return _mm_load_si128(reinterpret_cast<const __m128i*>(Array));
    }

    FORCEINLINE Int128 VECTORCALL Load(int x, int y, int z, int w) noexcept
    {
        return _mm_set_epi32(w, z, y, x);
    }

    FORCEINLINE Int128 VECTORCALL Load(int Scalar) noexcept
    {
        return _mm_set1_epi32(Scalar);
    }

    FORCEINLINE Float128 VECTORCALL CastIntToFloat(Int128 A) noexcept
    {
        return _mm_castsi128_ps(A);
    }

    FORCEINLINE Int128 VECTORCALL CastFloatToInt(Float128 A) noexcept
    {
        return _mm_castps_si128(A);
    }

    FORCEINLINE void VECTORCALL StoreAligned(Float128 Register, float* Array) noexcept
    {
        return _mm_store_ps(Array, Register);
    }

    template<uint8 x, uint8 y, uint8 z, uint8 w>
    FORCEINLINE Float128 VECTORCALL Shuffle(Float128 A) noexcept
    {
        return _mm_shuffle_ps(A, A, _MM_SHUFFLE(w, z, y, x));
    }

    template<>
    FORCEINLINE Float128 VECTORCALL Shuffle<0, 1, 0, 1>(Float128 A) noexcept
    {
        return _mm_movelh_ps(A, A);
    }

    template<>
    FORCEINLINE Float128 VECTORCALL Shuffle<2, 3, 2, 3>(Float128 A) noexcept
    {
        return _mm_movehl_ps(A, A);
    }

    template<>
    FORCEINLINE Float128 VECTORCALL Shuffle<0, 0, 1, 1>(Float128 A) noexcept
    {
        return _mm_unpacklo_ps(A, A);
    }

    template<>
    FORCEINLINE Float128 VECTORCALL Shuffle<2, 2, 3, 3>(Float128 A) noexcept
    {
        return _mm_unpackhi_ps(A, A);
    }

    template<uint8 x, uint8 y, uint8 z, uint8 w>
    FORCEINLINE Float128 VECTORCALL Shuffle0011(Float128 A, Float128 B) noexcept
    {
        return _mm_shuffle_ps(A, B, _MM_SHUFFLE(w, z, y, x));
    }

    template<>
    FORCEINLINE Float128 VECTORCALL Shuffle0011<0, 1, 0, 1>(Float128 A, Float128 B) noexcept
    {
        return _mm_movelh_ps(A, B);
    }

    template<>
    FORCEINLINE Float128 VECTORCALL Shuffle0011<2, 3, 2, 3>(Float128 A, Float128 B) noexcept
    {
        return _mm_movehl_ps(B, A);
    }

    template<uint8 x, uint8 y, uint8 z, uint8 w>
    FORCEINLINE Float128 VECTORCALL Shuffle0101(Float128 A, Float128 B) noexcept
    {
        Float128 TempA = Shuffle0011<x, z, y, w>(A, B);
        return Shuffle<0, 2, 1, 3>(TempA);
    }

    template<>
    FORCEINLINE Float128 VECTORCALL Shuffle0101<0, 0, 1, 1>(Float128 A, Float128 B) noexcept
    {
        return _mm_unpacklo_ps(A, B);
    }

    template<>
    FORCEINLINE Float128 VECTORCALL Shuffle0101<2, 2, 3, 3>(Float128 A, Float128 B) noexcept
    {
        return _mm_unpackhi_ps(A, B);
    }

    FORCEINLINE float VECTORCALL GetX(Float128 Register) noexcept
    {
        return _mm_cvtss_f32(Register);
    }

    FORCEINLINE Float128 VECTORCALL Mul(Float128 A, Float128 B) noexcept
    {
        return _mm_mul_ps(A, B);
    }

    FORCEINLINE Float128 VECTORCALL Div(Float128 A, Float128 B) noexcept
    {
        return _mm_div_ps(A, B);
    }

    FORCEINLINE Float128 VECTORCALL Add(Float128 A, Float128 B) noexcept
    {
        return _mm_add_ps(A, B);
    }

    FORCEINLINE Float128 VECTORCALL HorizontalAdd(Float128 A, Float128 B) noexcept
    {
#if USE_SSE3
        return _mm_hadd_ps(A, B);
#else
        Float128 Temp0 = Shuffle0011<0, 2, 0, 2>(A, B);
        Float128 Temp1 = Shuffle0011<1, 3, 1, 3>(A, B);
        return Add(Temp0, Temp1);
#endif
    }

    FORCEINLINE Float128 VECTORCALL Sub(Float128 A, Float128 B) noexcept
    {
        return _mm_sub_ps(A, B);
    }

    FORCEINLINE Float128 VECTORCALL Sqrt(Float128 A) noexcept
    {
        return _mm_sqrt_ps(A);
    }

    FORCEINLINE Float128 VECTORCALL RecipSqrt(Float128 A) noexcept
    {
        return _mm_rsqrt_ps(A);
    }

    FORCEINLINE Float128 VECTORCALL Recip(Float128 A) noexcept
    {
        return _mm_rcp_ps(A);
    }

    FORCEINLINE Float128 VECTORCALL And(Float128 A, Float128 B) noexcept
    {
        return _mm_and_ps(A, B);
    }

    FORCEINLINE Float128 VECTORCALL Or(Float128 A, Float128 B) noexcept
    {
        return _mm_or_ps(A, B);
    }

    FORCEINLINE bool VECTORCALL Equal(Float128 A, Float128 B) noexcept
    {
        const auto Mask = _mm_movemask_ps(_mm_cmpeq_ps(A, B));
        return 0xf == (Mask & 0xf);
    }

    FORCEINLINE bool VECTORCALL GreaterThan(Float128 A, Float128 B) noexcept
    {
        const auto Mask = _mm_movemask_ps(_mm_cmpgt_ps(A, B));
        return 0xf == (Mask & 0xf);
    }

    FORCEINLINE bool VECTORCALL GreaterThanOrEqual(Float128 A, Float128 B) noexcept
    {
        const auto Mask = _mm_movemask_ps(_mm_cmpge_ps(A, B));
        return 0xf == (Mask & 0xf);
    }

    FORCEINLINE bool VECTORCALL LessThan(Float128 A, Float128 B) noexcept
    {
        const auto Mask = _mm_movemask_ps(_mm_cmplt_ps(A, B));
        return 0xf == (Mask & 0xf);
    }

    FORCEINLINE bool VECTORCALL LessThanOrEqual(Float128 A, Float128 B) noexcept
    {
        const auto Mask = _mm_movemask_ps(_mm_cmple_ps(A, B));
        return 0xf == (Mask & 0xf);
    }

    FORCEINLINE Float128 VECTORCALL Min(Float128 A, Float128 B) noexcept
    {
        return _mm_min_ps(A, B);
    }

    FORCEINLINE Float128 VECTORCALL Max(Float128 A, Float128 B) noexcept
    {
        return _mm_max_ps(A, B);
    }
#endif

    template<typename T>
    FORCEINLINE Float128 VECTORCALL LoadAligned(const T* Object) noexcept
    {
        return LoadAligned(reinterpret_cast<const float*>(Object));
    }

    template<typename T>
    FORCEINLINE void VECTORCALL StoreAligned(Float128 Register, T* Object) noexcept
    {
        StoreAligned(Register, reinterpret_cast<float*>(Object));
    }

    template<uint8 i>
    FORCEINLINE Float128 VECTORCALL Broadcast(Float128 Register) noexcept
    {
        return Shuffle<i, i, i, i>(Register);
    }

    FORCEINLINE Float128 VECTORCALL Mul(const float* A, Float128 B) noexcept
    {
        Float128 Temp = LoadAligned(A);
        return Mul(Temp, B);
    }

    template<typename T>
    FORCEINLINE Float128 VECTORCALL Mul(const T* A, Float128 B) noexcept
    {
        return Mul(reinterpret_cast<const float*>(A), B);
    }

    FORCEINLINE Float128 VECTORCALL Mul(Float128 A, const float* B) noexcept
    {
        Float128 Temp = LoadAligned(B);
        return Mul(A, Temp);
    }

    template<typename T>
    FORCEINLINE Float128 VECTORCALL Mul(Float128 A, const T* B) noexcept
    {
        return Mul(A, reinterpret_cast<const float*>(B));
    }

    FORCEINLINE Float128 VECTORCALL Mul(const float* A, const float* B) noexcept
    {
        Float128 Temp0 = LoadAligned(A);
        Float128 Temp1 = LoadAligned(B);
        return Mul(Temp0, Temp1);
    }

    template<typename T>
    FORCEINLINE Float128 VECTORCALL Mul(const T* A, const T* B) noexcept
    {
        return Mul(reinterpret_cast<const float*>(A), reinterpret_cast<const float*>(B));
    }

    FORCEINLINE Float128 VECTORCALL Div(const float* A, Float128 B) noexcept
    {
        Float128 Temp = LoadAligned(A);
        return Div(Temp, B);
    }

    template<typename T>
    FORCEINLINE Float128 VECTORCALL Div(const T* A, Float128 B) noexcept
    {
        return Div(reinterpret_cast<const float*>(A), B);
    }

    FORCEINLINE Float128 VECTORCALL Div(Float128 A, const float* B) noexcept
    {
        Float128 Temp = LoadAligned(B);
        return Div(A, Temp);
    }

    template<typename T>
    FORCEINLINE Float128 VECTORCALL Div(Float128 A, const T* B) noexcept
    {
        return Div(A, reinterpret_cast<const float*>(B));
    }

    FORCEINLINE Float128 VECTORCALL Div(const float* A, const float* B) noexcept
    {
        Float128 Temp0 = LoadAligned(A);
        Float128 Temp1 = LoadAligned(B);
        return Div(Temp0, Temp1);
    }

    template<typename T>
    FORCEINLINE Float128 VECTORCALL Div(const T* A, const T* B) noexcept
    {
        return Div(reinterpret_cast<const float*>(A), reinterpret_cast<const float*>(B));
    }

    FORCEINLINE Float128 VECTORCALL Add(const float* A, Float128 B) noexcept
    {
        Float128 Temp = LoadAligned(A);
        return Add(Temp, B);
    }

    template<typename T>
    FORCEINLINE Float128 VECTORCALL Add(const T* A, Float128 B) noexcept
    {
        return Add(reinterpret_cast<const float*>(A), B);
    }

    FORCEINLINE Float128 VECTORCALL Add(Float128 A, const float* B) noexcept
    {
        Float128 Temp = LoadAligned(B);
        return Add(A, Temp);
    }

    template<typename T>
    FORCEINLINE Float128 VECTORCALL Add(Float128 A, const T* B) noexcept
    {
        return Add(A, reinterpret_cast<const float*>(B));
    }

    FORCEINLINE Float128 VECTORCALL Add(const float* A, const float* B) noexcept
    {
        Float128 Temp0 = LoadAligned(A);
        Float128 Temp1 = LoadAligned(B);
        return Add(Temp0, Temp1);
    }

    template<typename T>
    FORCEINLINE Float128 VECTORCALL Add(const T* A, const T* B) noexcept
    {
        return Add(reinterpret_cast<const float*>(A), reinterpret_cast<const float*>(B));
    }

    FORCEINLINE Float128 VECTORCALL Sub(const float* A, Float128 B) noexcept
    {
        Float128 Temp = LoadAligned(A);
        return Sub(Temp, B);
    }

    template<typename T>
    FORCEINLINE Float128 VECTORCALL Sub(const T* A, Float128 B) noexcept
    {
        return Sub(reinterpret_cast<const float*>(A), B);
    }

    FORCEINLINE Float128 VECTORCALL Sub(Float128 A, const float* B) noexcept
    {
        Float128 Temp = LoadAligned(B);
        return Sub(A, Temp);
    }

    template<typename T>
    FORCEINLINE Float128 VECTORCALL Sub(Float128 A, const T* B) noexcept
    {
        return Sub(A, reinterpret_cast<const float*>(B));
    }

    FORCEINLINE Float128 VECTORCALL Sub(const float* A, const float* B) noexcept
    {
        Float128 Temp0 = LoadAligned(A);
        Float128 Temp1 = LoadAligned(B);
        return Sub(Temp0, Temp1);
    }

    template<typename T>
    FORCEINLINE Float128 VECTORCALL Sub(const T* A, const T* B) noexcept
    {
        return Sub(reinterpret_cast<const float*>(A), reinterpret_cast<const float*>(B));
    }

    FORCEINLINE float VECTORCALL GetY(Float128 Register) noexcept
    {
        Float128 Temp = Broadcast<1>(Register);
        return GetX(Temp);
    }

    FORCEINLINE float VECTORCALL GetZ(Float128 Register) noexcept
    {
        Float128 Temp = Broadcast<2>(Register);
        return GetX(Temp);
    }

    FORCEINLINE float VECTORCALL GetW(Float128 Register) noexcept
    {
        Float128 Temp = Broadcast<3>(Register);
        return GetX(Temp);
    }

    FORCEINLINE Float128 VECTORCALL Abs(Float128 A) noexcept
    {
        Int128 Mask = Load(~(1 << 31));
        return And(A, CastIntToFloat(Mask));
    }

    FORCEINLINE Float128 VECTORCALL Dot(Float128 A, Float128 B) noexcept
    {
        Float128 Reg0 = Mul(A, B);                  // (Ax*Bx), (Ay*By)...
        Float128 Reg1 = Shuffle<1, 0, 3, 2>(Reg0);  // (Ay*By), (Ax*Bx), (Aw*Bw), (Az*Bz))
        Reg0 = Add(Reg0, Reg1);                     // (Ax * Bx) + (Ay * By), ..., (Az * Bz) + (Aw * Bw)...
        Reg1 = Shuffle0011<2, 3, 2, 3>(Reg0, Reg1); // (Az * Bz) + (Aw * Bw), ..., (Ay*By), (Ax*Bx)
        return Add(Reg0, Reg1);                     // (Ax * Bx) + (Ay * By) + (Az * Bz) + (Aw * Bw), ...
    }

    FORCEINLINE Float128 VECTORCALL Cross(Float128 A, Float128 B) noexcept
    {
        Float128 Mul0 = Shuffle<1, 2, 0, 3>(A);
        Mul0 = Mul(B, Mul0);

        Float128 Mul1 = Shuffle<1, 2, 0, 3>(B);
        Mul1 = Mul(A, Mul1);

        Float128 Result = Sub(Mul1, Mul0);
        return Shuffle<1, 2, 0, 3>(Result);
    }

    FORCEINLINE Float128 VECTORCALL Transform(const float* Mat, Float128 Row) noexcept
    {
        Float128 Reg0 = LoadAligned(&Mat[0]);
        Float128 Reg1 = Mul(Broadcast<0>(Row), Reg0);

        Reg0 = LoadAligned(&Mat[4]);
        Reg1 = Add(Reg1, Mul(Broadcast<1>(Row), Reg0));

        Reg0 = LoadAligned(&Mat[8]);
        Reg1 = Add(Reg1, Mul(Broadcast<2>(Row), Reg0));

        Reg0 = LoadAligned(&Mat[12]);
        return Add(Reg1, Mul(Broadcast<3>(Row), Reg0));
    }

    template<typename T>
    FORCEINLINE Float128 VECTORCALL Transform(const T* Mat, Float128 Row) noexcept
    {
        return Transform(reinterpret_cast<const float*>(Mat), Row);
    }

    FORCEINLINE void VECTORCALL Transpose(const float* InMat, float* OutMat) noexcept
    {
        Float128 Row0  = LoadAligned(&InMat[0]);
        Float128 Row1  = LoadAligned(&InMat[4]);
        Float128 Temp0 = Shuffle0101<0, 0, 1, 1>(Row0, Row1);
        Float128 Temp1 = Shuffle0101<2, 2, 3, 3>(Row0, Row1);

        Row0 = LoadAligned(&InMat[8]);
        Row1 = LoadAligned(&InMat[12]);

        Float128 Temp2 = Shuffle0101<0, 0, 1, 1>(Row0, Row1);
        Float128 Temp3 = Shuffle0101<2, 2, 3, 3>(Row0, Row1);

        Float128 Out = Shuffle0011<0, 1, 0, 1>(Temp0, Temp2);
        StoreAligned(Out, &OutMat[0]);

        Out = Shuffle0011<2, 3, 2, 3>(Temp0, Temp2);
        StoreAligned(Out, &OutMat[4]);

        Out = Shuffle0011<0, 1, 0, 1>(Temp1, Temp3);
        StoreAligned(Out, &OutMat[8]);

        Out = Shuffle0011<2, 3, 2, 3>(Temp1, Temp3);
        StoreAligned(Out, &OutMat[12]);
    }

    template<typename T>
    FORCEINLINE void VECTORCALL Transpose(const T* InMat, T* OutMat) noexcept
    {
        return Transpose(reinterpret_cast<const float*>(InMat), reinterpret_cast<float*>(OutMat));
    }

    FORCEINLINE Float128 VECTORCALL HorizontalAdd(Float128 A) noexcept
    {
        return HorizontalAdd(A, A);
    }

    FORCEINLINE Float128 VECTORCALL HorizontalSum(Float128 A) noexcept
    {
        Float128 Shf = Shuffle<1, 0, 3, 2>(A);
        Float128 Sum = Add(A, Shf);
        Shf = Shuffle0011<2, 3, 2, 3>(Shf, Sum);
        Sum = Add(Shf, Sum);
        return Broadcast<0>(Sum);
    }

    FORCEINLINE Float128 VECTORCALL Mat2Mul(Float128 A, Float128 B)
    {
        Float128 Temp0 = Shuffle<0, 3, 0, 3>(B);
        Float128 Temp1 = Mul(A, Temp0);
        Float128 Temp2 = Shuffle<1, 0, 3, 2>(A);
        Temp0 = Shuffle<2, 1, 2, 1>(B);

        Float128 Temp3 = Mul(Temp2, Temp0);
        return Add(Temp1, Temp3);
    }

    FORCEINLINE Float128 VECTORCALL Mat2AdjointMul(Float128 A, Float128 B)
    {
        Float128 Temp0 = Shuffle<1, 1, 2, 2>(A);
        Float128 Temp1 = Shuffle<2, 3, 0, 1>(B);
        Float128 Temp2 = Mul(Temp0, Temp1);
        Temp0 = Shuffle<3, 3, 0, 0>(A);
        Temp1 = Mul(Temp0, B);
        return Sub(Temp1, Temp2);
    }

    FORCEINLINE Float128 VECTORCALL Mat2MulAdjoint(Float128 A, Float128 B)
    {
        Float128 Temp0 = Shuffle<1, 0, 3, 2>(A);
        Float128 Temp1 = Shuffle<2, 1, 2, 1>(B);
        Float128 Temp2 = Mul(Temp0, Temp1);
        Temp0 = Shuffle<3, 0, 3, 0>(B);
        Temp1 = Mul(A, Temp0);
        return Sub(Temp1, Temp2);
    }
}
#else
    #define USE_VECTOR_OP (0)
    #error No SIMD
#endif

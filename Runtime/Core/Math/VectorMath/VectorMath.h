#pragma once
#include "Core/Math/MathCommon.h"

#ifndef VECTOR_ALIGN
    #define VECTOR_ALIGN ALIGN_AS(16)
#endif

#if PLATFORM_ARCHITECTURE_X86_X64 && PLATFORM_SUPPORT_SSE_INTRIN
    #define USE_VECTOR_MATH (1)

    #if PLATFORM_SUPPORT_SSE4_2_INTRIN
        #include "Core/Math/VectorMath/VectorMathSSE4_2.h"
        typedef FVectorMathSSE4_2 FPlatformVectorMath;
    #elif PLATFORM_SUPPORT_SSE4_1_INTRIN
        #include "Core/Math/VectorMath/VectorMathSSE4_1.h"
        typedef FVectorMathSSE4_1 FPlatformVectorMath;
    #elif PLATFORM_SUPPORT_SSSE3_INTRIN
        #include "Core/Math/VectorMath/VectorMathSSSE3.h"
        typedef FVectorMathSSSE3 FPlatformVectorMath;
    #elif PLATFORM_SUPPORT_SSE3_INTRIN
        #include "Core/Math/VectorMath/VectorMathSSE3.h"
        typedef FVectorMathSSE3 FPlatformVectorMath;
    #elif PLATFORM_SUPPORT_SSE2_INTRIN
        #include "Core/Math/VectorMath/VectorMathSSE2.h"
        typedef FVectorMathSSE2 FPlatformVectorMath;
    #else
        #include "Core/Math/VectorMath/VectorMathSSE.h"
        typedef FVectorMathSSE FPlatformVectorMath;
    #endif
#else
    #define USE_VECTOR_MATH (0)

    // TODO: Add fallback when we have no SSE intrinsics
    #include "Core/Math/VectorMath/GenericVectorMath.h"
    typedef FGenericVectorMath FPlatformVectorMath;
#endif

struct FVectorMath : public FPlatformVectorMath
{
    using FPlatformVectorMath::Mul;
    using FPlatformVectorMath::Div;
    using FPlatformVectorMath::Add;
    using FPlatformVectorMath::Sub;

    template<uint8 RegisterIndex>
    static FORCEINLINE FFloat128 VECTORCALL Broadcast(FFloat128 Vector) noexcept
    {
        return FPlatformVectorMath::Shuffle<RegisterIndex, RegisterIndex, RegisterIndex, RegisterIndex>(Vector);
    }

    static FORCEINLINE FFloat128 VECTORCALL Mul(const float* A, FFloat128 VectorB) noexcept
    {
        FFloat128 Temp = FPlatformVectorMath::LoadAligned(A);
        return FPlatformVectorMath::Mul(Temp, VectorB);
    }

    template<typename T>
    static FORCEINLINE FFloat128 VECTORCALL Mul(const T* A, FFloat128 VectorB) noexcept
    {
        return Mul(reinterpret_cast<const float*>(A), VectorB);
    }

    static FORCEINLINE FFloat128 VECTORCALL Mul(FFloat128 A, const float* B) noexcept
    {
        FFloat128 Temp = FPlatformVectorMath::LoadAligned(B);
        return FPlatformVectorMath::Mul(A, Temp);
    }

    template<typename T>
    static FORCEINLINE FFloat128 VECTORCALL Mul(FFloat128 A, const T* B) noexcept
    {
        return Mul(A, reinterpret_cast<const float*>(B));
    }

    static FORCEINLINE FFloat128 VECTORCALL Mul(const float* A, const float* B) noexcept
    {
        FFloat128 Temp0 = FPlatformVectorMath::LoadAligned(A);
        FFloat128 Temp1 = FPlatformVectorMath::LoadAligned(B);
        return FPlatformVectorMath::Mul(Temp0, Temp1);
    }

    template<typename T>
    static FORCEINLINE FFloat128 VECTORCALL Mul(const T* A, const T* B) noexcept
    {
        return Mul(reinterpret_cast<const float*>(A), reinterpret_cast<const float*>(B));
    }

    static FORCEINLINE FFloat128 VECTORCALL Div(const float* A, FFloat128 B) noexcept
    {
        FFloat128 Temp = FPlatformVectorMath::LoadAligned(A);
        return FPlatformVectorMath::Div(Temp, B);
    }

    template<typename T>
    static FORCEINLINE FFloat128 VECTORCALL Div(const T* A, FFloat128 B) noexcept
    {
        return Div(reinterpret_cast<const float*>(A), B);
    }

    static FORCEINLINE FFloat128 VECTORCALL Div(FFloat128 A, const float* B) noexcept
    {
        FFloat128 Temp = FPlatformVectorMath::LoadAligned(B);
        return FPlatformVectorMath::Div(A, Temp);
    }

    template<typename T>
    static FORCEINLINE FFloat128 VECTORCALL Div(FFloat128 A, const T* B) noexcept
    {
        return Div(A, reinterpret_cast<const float*>(B));
    }

    static FORCEINLINE FFloat128 VECTORCALL Div(const float* A, const float* B) noexcept
    {
        FFloat128 Temp0 = FPlatformVectorMath::LoadAligned(A);
        FFloat128 Temp1 = FPlatformVectorMath::LoadAligned(B);
        return FPlatformVectorMath::Div(Temp0, Temp1);
    }

    template<typename T>
    static FORCEINLINE FFloat128 VECTORCALL Div(const T* A, const T* B) noexcept
    {
        return Div(reinterpret_cast<const float*>(A), reinterpret_cast<const float*>(B));
    }

    static FORCEINLINE FFloat128 VECTORCALL Add(const float* A, FFloat128 B) noexcept
    {
        FFloat128 Temp = FPlatformVectorMath::LoadAligned(A);
        return FPlatformVectorMath::Add(Temp, B);
    }

    template<typename T>
    static FORCEINLINE FFloat128 VECTORCALL Add(const T* A, FFloat128 B) noexcept
    {
        return Add(reinterpret_cast<const float*>(A), B);
    }

    static FORCEINLINE FFloat128 VECTORCALL Add(FFloat128 A, const float* B) noexcept
    {
        FFloat128 Temp = FPlatformVectorMath::LoadAligned(B);
        return FPlatformVectorMath::Add(A, Temp);
    }

    template<typename T>
    static FORCEINLINE FFloat128 VECTORCALL Add(FFloat128 A, const T* B) noexcept
    {
        return Add(A, reinterpret_cast<const float*>(B));
    }

    static FORCEINLINE FFloat128 VECTORCALL Add(const float* A, const float* B) noexcept
    {
        FFloat128 Temp0 = FPlatformVectorMath::LoadAligned(A);
        FFloat128 Temp1 = FPlatformVectorMath::LoadAligned(B);
        return FPlatformVectorMath::Add(Temp0, Temp1);
    }

    template<typename T>
    static FORCEINLINE FFloat128 VECTORCALL Add(const T* A, const T* B) noexcept
    {
        return Add(reinterpret_cast<const float*>(A), reinterpret_cast<const float*>(B));
    }

    static FORCEINLINE FFloat128 VECTORCALL Sub(const float* A, FFloat128 B) noexcept
    {
        FFloat128 Temp = LoadAligned(A);
        return FPlatformVectorMath::Sub(Temp, B);
    }

    template<typename T>
    static FORCEINLINE FFloat128 VECTORCALL Sub(const T* A, FFloat128 B) noexcept
    {
        return Sub(reinterpret_cast<const float*>(A), B);
    }

    static FORCEINLINE FFloat128 VECTORCALL Sub(FFloat128 A, const float* B) noexcept
    {
        FFloat128 Temp = FPlatformVectorMath::LoadAligned(B);
        return FPlatformVectorMath::Sub(A, Temp);
    }

    template<typename T>
    static FORCEINLINE FFloat128 VECTORCALL Sub(FFloat128 A, const T* B) noexcept
    {
        return Sub(A, reinterpret_cast<const float*>(B));
    }

    static FORCEINLINE FFloat128 VECTORCALL Sub(const float* A, const float* B) noexcept
    {
        FFloat128 Temp0 = FPlatformVectorMath::LoadAligned(A);
        FFloat128 Temp1 = FPlatformVectorMath::LoadAligned(B);
        return FPlatformVectorMath::Sub(Temp0, Temp1);
    }

    template<typename T>
    static FORCEINLINE FFloat128 VECTORCALL Sub(const T* A, const T* B) noexcept
    {
        return Sub(reinterpret_cast<const float*>(A), reinterpret_cast<const float*>(B));
    }

    static FORCEINLINE float VECTORCALL GetY(FFloat128 Register) noexcept
    {
        FFloat128 Temp = Broadcast<1>(Register);
        return GetX(Temp);
    }

    static FORCEINLINE float VECTORCALL GetZ(FFloat128 Register) noexcept
    {
        FFloat128 Temp = Broadcast<2>(Register);
        return GetX(Temp);
    }

    static FORCEINLINE float VECTORCALL GetW(FFloat128 Register) noexcept
    {
        FFloat128 Temp = Broadcast<3>(Register);
        return GetX(Temp);
    }

    static FORCEINLINE FFloat128 VECTORCALL Abs(FFloat128 A) noexcept
    {
        FInt128 Mask = Load(~(1 << 31));
        return And(A, CastIntToFloat(Mask));
    }

    static FORCEINLINE FFloat128 VECTORCALL Dot(FFloat128 A, FFloat128 B) noexcept
    {
        FFloat128 Reg0 = FPlatformVectorMath::Mul(A, B);                 // (Ax*Bx), (Ay*By)...
        FFloat128 Reg1 = FPlatformVectorMath::Shuffle<1, 0, 3, 2>(Reg0); // (Ay*By), (Ax*Bx), (Aw*Bw), (Az*Bz))
        Reg0 = FPlatformVectorMath::Add(Reg0, Reg1);                     // (Ax * Bx) + (Ay * By), ..., (Az * Bz) + (Aw * Bw)...
        Reg1 = FPlatformVectorMath::Shuffle0011<2, 3, 2, 3>(Reg0, Reg1); // (Az * Bz) + (Aw * Bw), ..., (Ay*By), (Ax*Bx)
        return FPlatformVectorMath::Add(Reg0, Reg1);                     // (Ax * Bx) + (Ay * By) + (Az * Bz) + (Aw * Bw), ...
    }

    static FORCEINLINE FFloat128 VECTORCALL Cross(FFloat128 A, FFloat128 B) noexcept
    {
        FFloat128 Mul0 = Shuffle<1, 2, 0, 3>(A);
        Mul0 = FPlatformVectorMath::Mul(B, Mul0);

        FFloat128 Mul1 = Shuffle<1, 2, 0, 3>(B);
        Mul1 = FPlatformVectorMath::Mul(A, Mul1);

        FFloat128 Result = FPlatformVectorMath::Sub(Mul1, Mul0);
        return Shuffle<1, 2, 0, 3>(Result);
    }

    static FORCEINLINE FFloat128 VECTORCALL Transform(const float* Mat, FFloat128 Row) noexcept
    {
        FFloat128 Reg0 = FPlatformVectorMath::LoadAligned(&Mat[0]);
        FFloat128 Reg1 = FPlatformVectorMath::Mul(Broadcast<0>(Row), Reg0);

        Reg0 = FPlatformVectorMath::LoadAligned(&Mat[4]);
        Reg1 = FPlatformVectorMath::Add(Reg1, FPlatformVectorMath::Mul(Broadcast<1>(Row), Reg0));

        Reg0 = FPlatformVectorMath::LoadAligned(&Mat[8]);
        Reg1 = FPlatformVectorMath::Add(Reg1, FPlatformVectorMath::Mul(Broadcast<2>(Row), Reg0));

        Reg0 = FPlatformVectorMath::LoadAligned(&Mat[12]);
        return FPlatformVectorMath::Add(Reg1, FPlatformVectorMath::Mul(Broadcast<3>(Row), Reg0));
    }

    template<typename T>
    static FORCEINLINE FFloat128 VECTORCALL Transform(const T* Mat, FFloat128 Row) noexcept
    {
        return Transform(reinterpret_cast<const float*>(Mat), Row);
    }

    static FORCEINLINE void VECTORCALL Transpose(const float* InMat, float* OutMat) noexcept
    {
        FFloat128 Row0  = FPlatformVectorMath::LoadAligned(&InMat[0]);
        FFloat128 Row1  = FPlatformVectorMath::LoadAligned(&InMat[4]);
        FFloat128 Temp0 = FPlatformVectorMath::Shuffle0101<0, 0, 1, 1>(Row0, Row1);
        FFloat128 Temp1 = FPlatformVectorMath::Shuffle0101<2, 2, 3, 3>(Row0, Row1);

        Row0 = FPlatformVectorMath::LoadAligned(&InMat[8]);
        Row1 = FPlatformVectorMath::LoadAligned(&InMat[12]);

        FFloat128 Temp2 = FPlatformVectorMath::Shuffle0101<0, 0, 1, 1>(Row0, Row1);
        FFloat128 Temp3 = FPlatformVectorMath::Shuffle0101<2, 2, 3, 3>(Row0, Row1);

        FFloat128 Out = FPlatformVectorMath::Shuffle0011<0, 1, 0, 1>(Temp0, Temp2);
        FPlatformVectorMath::StoreAligned(Out, &OutMat[0]);

        Out = FPlatformVectorMath::Shuffle0011<2, 3, 2, 3>(Temp0, Temp2);
        FPlatformVectorMath::StoreAligned(Out, &OutMat[4]);

        Out = FPlatformVectorMath::Shuffle0011<0, 1, 0, 1>(Temp1, Temp3);
        FPlatformVectorMath::StoreAligned(Out, &OutMat[8]);

        Out = FPlatformVectorMath::Shuffle0011<2, 3, 2, 3>(Temp1, Temp3);
        FPlatformVectorMath::StoreAligned(Out, &OutMat[12]);
    }

    template<typename T>
    static FORCEINLINE void VECTORCALL Transpose(const T* InMat, T* OutMat) noexcept
    {
        return Transpose(reinterpret_cast<const float*>(InMat), reinterpret_cast<float*>(OutMat));
    }

    static FORCEINLINE FFloat128 VECTORCALL HorizontalAdd(FFloat128 A) noexcept
    {
        return FPlatformVectorMath::HorizontalAdd(A, A);
    }

    static FORCEINLINE FFloat128 VECTORCALL HorizontalSum(FFloat128 A) noexcept
    {
        FFloat128 Shf = FPlatformVectorMath::Shuffle<1, 0, 3, 2>(A);
        FFloat128 Sum = FPlatformVectorMath::Add(A, Shf);
        Shf = FPlatformVectorMath::Shuffle0011<2, 3, 2, 3>(Shf, Sum);
        Sum = FPlatformVectorMath::Add(Shf, Sum);
        return Broadcast<0>(Sum);
    }

    static FORCEINLINE FFloat128 VECTORCALL Mat2Mul(FFloat128 A, FFloat128 B)
    {
        FFloat128 Temp0 = FPlatformVectorMath::Shuffle<0, 3, 0, 3>(B);
        FFloat128 Temp1 = FPlatformVectorMath::Mul(A, Temp0);
        FFloat128 Temp2 = FPlatformVectorMath::Shuffle<1, 0, 3, 2>(A);
        Temp0 = FPlatformVectorMath::Shuffle<2, 1, 2, 1>(B);

        FFloat128 Temp3 = FPlatformVectorMath::Mul(Temp2, Temp0);
        return FPlatformVectorMath::Add(Temp1, Temp3);
    }

    static FORCEINLINE FFloat128 VECTORCALL Mat2AdjointMul(FFloat128 A, FFloat128 B)
    {
        FFloat128 Temp0 = FPlatformVectorMath::Shuffle<1, 1, 2, 2>(A);
        FFloat128 Temp1 = FPlatformVectorMath::Shuffle<2, 3, 0, 1>(B);
        FFloat128 Temp2 = FPlatformVectorMath::Mul(Temp0, Temp1);
        Temp0 = FPlatformVectorMath::Shuffle<3, 3, 0, 0>(A);
        Temp1 = FPlatformVectorMath::Mul(Temp0, B);
        return FPlatformVectorMath::Sub(Temp1, Temp2);
    }

    static FORCEINLINE FFloat128 VECTORCALL Mat2MulAdjoint(FFloat128 A, FFloat128 B)
    {
        FFloat128 Temp0 = FPlatformVectorMath::Shuffle<1, 0, 3, 2>(A);
        FFloat128 Temp1 = FPlatformVectorMath::Shuffle<2, 1, 2, 1>(B);
        FFloat128 Temp2 = FPlatformVectorMath::Mul(Temp0, Temp1);
        Temp0 = FPlatformVectorMath::Shuffle<3, 0, 3, 0>(B);
        Temp1 = FPlatformVectorMath::Mul(A, Temp0);
        return FPlatformVectorMath::Sub(Temp1, Temp2);
    }
};

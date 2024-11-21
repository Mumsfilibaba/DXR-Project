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
    using FPlatformVectorMath::VectorMul;
    using FPlatformVectorMath::VectorDiv;
    using FPlatformVectorMath::VectorAdd;
    using FPlatformVectorMath::VectorSub;

    template<uint8 RegisterIndex>
    static FORCEINLINE FFloat128 VECTORCALL VectorBroadcast(FFloat128 Vector) noexcept
    {
        return VectorShuffle<RegisterIndex, RegisterIndex, RegisterIndex, RegisterIndex>(Vector);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorMul(const float* VectorA, FFloat128 VectorB) noexcept
    {
        FFloat128 VectorA_128 = LoadAligned(VectorA);
        return VectorMul(VectorA_128, VectorB);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorMul(FFloat128 VectorA, const float* VectorB) noexcept
    {
        FFloat128 VectorB_128 = LoadAligned(VectorB);
        return VectorMul(VectorA, VectorB_128);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorMul(const float* VectorA, const float* VectorB) noexcept
    {
        FFloat128 VectorA_128 = LoadAligned(VectorA);
        FFloat128 VectorB_128 = LoadAligned(VectorB);
        return VectorMul(VectorA_128, VectorB_128);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorDiv(const float* VectorA, FFloat128 VectorB) noexcept
    {
        FFloat128 VectorA_128 = LoadAligned(VectorA);
        return VectorDiv(VectorA_128, VectorB);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorDiv(FFloat128 VectorA, const float* VectorB) noexcept
    {
        FFloat128 VectorB_128 = LoadAligned(VectorB);
        return VectorDiv(VectorA, VectorB_128);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorDiv(const float* VectorA, const float* VectorB) noexcept
    {
        FFloat128 VectorA_128 = LoadAligned(VectorA);
        FFloat128 VectorB_128 = LoadAligned(VectorB);
        return VectorDiv(VectorA_128, VectorB_128);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorAdd(const float* VectorA, FFloat128 VectorB) noexcept
    {
        FFloat128 VectorA_128 = LoadAligned(VectorA);
        return VectorAdd(VectorA_128, VectorB);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorAdd(FFloat128 VectorA, const float* VectorB) noexcept
    {
        FFloat128 VectorB_128 = LoadAligned(VectorB);
        return VectorAdd(VectorA, VectorB_128);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorAdd(const float* VectorA, const float* VectorB) noexcept
    {
        FFloat128 VectorA_128 = LoadAligned(VectorA);
        FFloat128 VectorB_128 = LoadAligned(VectorB);
        return VectorAdd(VectorA_128, VectorB_128);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSub(const float* VectorA, FFloat128 VectorB) noexcept
    {
        FFloat128 VectorA_128 = LoadAligned(VectorA);
        return VectorSub(VectorA_128, VectorB);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSub(FFloat128 VectorA, const float* VectorB) noexcept
    {
        FFloat128 VectorB_128 = LoadAligned(VectorB);
        return VectorSub(VectorA, VectorB_128);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSub(const float* VectorA, const float* VectorB) noexcept
    {
        FFloat128 VectorA_128 = LoadAligned(VectorA);
        FFloat128 VectorB_128 = LoadAligned(VectorB);
        return VectorSub(VectorA_128, VectorB_128);
    }

    static FORCEINLINE float VECTORCALL VectorGetY(FFloat128 Vector) noexcept
    {
        FFloat128 ComponentY = VectorBroadcast<1>(Vector);
        return VectorGetX(ComponentY);
    }

    static FORCEINLINE float VECTORCALL VectorGetZ(FFloat128 Vector) noexcept
    {
        FFloat128 ComponentZ = VectorBroadcast<2>(Vector);
        return VectorGetX(ComponentZ);
    }

    static FORCEINLINE float VECTORCALL VectorGetW(FFloat128 Vector) noexcept
    {
        FFloat128 ComponentW = VectorBroadcast<3>(Vector);
        return VectorGetX(ComponentW);
    }

    static FORCEINLINE FFloat128 VECTORCALL Abs(FFloat128 A) noexcept
    {
        FInt128 Mask = Load(~(1 << 31));
        return And(A, CastIntToFloat(Mask));
    }

    static FORCEINLINE FFloat128 VECTORCALL Dot(FFloat128 A, FFloat128 B) noexcept
    {
        FFloat128 Reg0 = VectorMul(A, B);                 // (Ax*Bx), (Ay*By)...
        FFloat128 Reg1 = VectorShuffle<1, 0, 3, 2>(Reg0); // (Ay*By), (Ax*Bx), (Aw*Bw), (Az*Bz))
        Reg0 = VectorAdd(Reg0, Reg1);                     // (Ax * Bx) + (Ay * By), ..., (Az * Bz) + (Aw * Bw)...
        Reg1 = VectorShuffle0011<2, 3, 2, 3>(Reg0, Reg1); // (Az * Bz) + (Aw * Bw), ..., (Ay*By), (Ax*Bx)
        return VectorAdd(Reg0, Reg1);                     // (Ax * Bx) + (Ay * By) + (Az * Bz) + (Aw * Bw), ...
    }

    static FORCEINLINE FFloat128 VECTORCALL Cross(FFloat128 A, FFloat128 B) noexcept
    {
        FFloat128 Mul0 = VectorShuffle<1, 2, 0, 3>(A);
        Mul0 = VectorMul(B, Mul0);

        FFloat128 Mul1 = VectorShuffle<1, 2, 0, 3>(B);
        Mul1 = VectorMul(A, Mul1);

        FFloat128 Result = VectorSub(Mul1, Mul0);
        return VectorShuffle<1, 2, 0, 3>(Result);
    }

    static FORCEINLINE FFloat128 VECTORCALL Transform(const float* Mat, FFloat128 Row) noexcept
    {
        FFloat128 Reg0 = LoadAligned(&Mat[0]);
        FFloat128 Reg1 = VectorMul(VectorBroadcast<0>(Row), Reg0);

        Reg0 = LoadAligned(&Mat[4]);
        Reg1 = VectorAdd(Reg1, VectorMul(VectorBroadcast<1>(Row), Reg0));

        Reg0 = LoadAligned(&Mat[8]);
        Reg1 = VectorAdd(Reg1, VectorMul(VectorBroadcast<2>(Row), Reg0));

        Reg0 = LoadAligned(&Mat[12]);
        return VectorAdd(Reg1, VectorMul(VectorBroadcast<3>(Row), Reg0));
    }

    template<typename T>
    static FORCEINLINE FFloat128 VECTORCALL Transform(const T* Mat, FFloat128 Row) noexcept
    {
        return Transform(reinterpret_cast<const float*>(Mat), Row);
    }

    static FORCEINLINE void VECTORCALL Transpose(const float* InMat, float* OutMat) noexcept
    {
        FFloat128 Row0  = LoadAligned(&InMat[0]);
        FFloat128 Row1  = LoadAligned(&InMat[4]);
        FFloat128 Temp0 = VectorShuffle0101<0, 0, 1, 1>(Row0, Row1);
        FFloat128 Temp1 = VectorShuffle0101<2, 2, 3, 3>(Row0, Row1);

        Row0 = LoadAligned(&InMat[8]);
        Row1 = LoadAligned(&InMat[12]);

        FFloat128 Temp2 = VectorShuffle0101<0, 0, 1, 1>(Row0, Row1);
        FFloat128 Temp3 = VectorShuffle0101<2, 2, 3, 3>(Row0, Row1);

        FFloat128 Out = VectorShuffle0011<0, 1, 0, 1>(Temp0, Temp2);
        FPlatformVectorMath::StoreAligned(Out, &OutMat[0]);

        Out = VectorShuffle0011<2, 3, 2, 3>(Temp0, Temp2);
        FPlatformVectorMath::StoreAligned(Out, &OutMat[4]);

        Out = VectorShuffle0011<0, 1, 0, 1>(Temp1, Temp3);
        FPlatformVectorMath::StoreAligned(Out, &OutMat[8]);

        Out = VectorShuffle0011<2, 3, 2, 3>(Temp1, Temp3);
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
        FFloat128 Shf = VectorShuffle<1, 0, 3, 2>(A);
        FFloat128 Sum = VectorAdd(A, Shf);
        Shf = VectorShuffle0011<2, 3, 2, 3>(Shf, Sum);
        Sum = VectorAdd(Shf, Sum);
        return VectorBroadcast<0>(Sum);
    }

    static FORCEINLINE FFloat128 VECTORCALL Mat2Mul(FFloat128 A, FFloat128 B)
    {
        FFloat128 Temp0 = VectorShuffle<0, 3, 0, 3>(B);
        FFloat128 Temp1 = VectorMul(A, Temp0);
        FFloat128 Temp2 = VectorShuffle<1, 0, 3, 2>(A);
        Temp0 = VectorShuffle<2, 1, 2, 1>(B);

        FFloat128 Temp3 = VectorMul(Temp2, Temp0);
        return VectorAdd(Temp1, Temp3);
    }

    static FORCEINLINE FFloat128 VECTORCALL Mat2AdjointMul(FFloat128 A, FFloat128 B)
    {
        FFloat128 Temp0 = VectorShuffle<1, 1, 2, 2>(A);
        FFloat128 Temp1 = VectorShuffle<2, 3, 0, 1>(B);
        FFloat128 Temp2 = VectorMul(Temp0, Temp1);
        Temp0 = VectorShuffle<3, 3, 0, 0>(A);
        Temp1 = VectorMul(Temp0, B);
        return VectorSub(Temp1, Temp2);
    }

    static FORCEINLINE FFloat128 VECTORCALL Mat2MulAdjoint(FFloat128 A, FFloat128 B)
    {
        FFloat128 Temp0 = VectorShuffle<1, 0, 3, 2>(A);
        FFloat128 Temp1 = VectorShuffle<2, 1, 2, 1>(B);
        FFloat128 Temp2 = VectorMul(Temp0, Temp1);
        Temp0 = VectorShuffle<3, 0, 3, 0>(B);
        Temp1 = VectorMul(A, Temp0);
        return VectorSub(Temp1, Temp2);
    }
};

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
    using FPlatformVectorMath::VectorHorizontalAdd;

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

    static FORCEINLINE FFloat128 VECTORCALL VectorAbs(FFloat128 Vector) noexcept
    {
        FInt128 Mask = Load(~(1 << 31));
        return And(Vector, VectorIntToFloat(Mask));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorDot(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        FFloat128 VectorC = VectorMul(VectorA, VectorB);           // (Ax*Bx), (Ay*By)...
        FFloat128 VectorD = VectorShuffle<1, 0, 3, 2>(VectorC);    // (Ay*By), (Ax*Bx), (Aw*Bw), (Az*Bz))

        VectorC = VectorAdd(VectorC, VectorD);                     // (Ax * Bx) + (Ay * By), ..., (Az * Bz) + (Aw * Bw)...
        VectorD = VectorShuffle0011<2, 3, 2, 3>(VectorC, VectorD); // (Az * Bz) + (Aw * Bw), ..., (Ay*By), (Ax*Bx)

        return VectorAdd(VectorC, VectorD);                        // (Ax * Bx) + (Ay * By) + (Az * Bz) + (Aw * Bw), ...
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorCross(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        FFloat128 VectorC = VectorShuffle<1, 2, 0, 3>(VectorA);
        VectorC = VectorMul(VectorB, VectorC);

        FFloat128 VectorD = VectorShuffle<1, 2, 0, 3>(VectorB);
        VectorD = VectorMul(VectorA, VectorD);

        FFloat128 Result = VectorSub(VectorD, VectorC);
        return VectorShuffle<1, 2, 0, 3>(Result);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorTransform(const float* Matrix, FFloat128 Vector) noexcept
    {
        FFloat128 VectorA = LoadAligned(&Matrix[0]);
        FFloat128 VectorB = VectorMul(VectorBroadcast<0>(Vector), VectorA);

        VectorA = LoadAligned(&Matrix[4]);
        VectorB = VectorAdd(VectorB, VectorMul(VectorBroadcast<1>(Vector), VectorA));

        VectorA = LoadAligned(&Matrix[8]);
        VectorB = VectorAdd(VectorB, VectorMul(VectorBroadcast<2>(Vector), VectorA));

        VectorA = LoadAligned(&Matrix[12]);
        return VectorAdd(VectorB, VectorMul(VectorBroadcast<3>(Vector), VectorA));
    }

    static FORCEINLINE void VECTORCALL VectorTranspose(const float* InMatrix, float* OutMatrix) noexcept
    {
        FFloat128 RowA    = LoadAligned(&InMatrix[0]);
        FFloat128 RowB    = LoadAligned(&InMatrix[4]);
        FFloat128 VectorA = VectorShuffle0101<0, 0, 1, 1>(RowA, RowB);
        FFloat128 VectorB = VectorShuffle0101<2, 2, 3, 3>(RowA, RowB);

        RowA = LoadAligned(&InMatrix[8]);
        RowB = LoadAligned(&InMatrix[12]);

        FFloat128 VectorC = VectorShuffle0101<0, 0, 1, 1>(RowA, RowB);
        FFloat128 VectorD = VectorShuffle0101<2, 2, 3, 3>(RowA, RowB);

        FFloat128 OutVector = VectorShuffle0011<0, 1, 0, 1>(VectorA, VectorC);
        StoreAligned(OutVector, &OutMatrix[0]);

        OutVector = VectorShuffle0011<2, 3, 2, 3>(VectorA, VectorC);
        StoreAligned(OutVector, &OutMatrix[4]);

        OutVector = VectorShuffle0011<0, 1, 0, 1>(VectorB, VectorD);
        StoreAligned(OutVector, &OutMatrix[8]);

        OutVector = VectorShuffle0011<2, 3, 2, 3>(VectorB, VectorD);
        StoreAligned(OutVector, &OutMatrix[12]);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorHorizontalAdd(FFloat128 Vector) noexcept
    {
        return VectorHorizontalAdd(Vector, Vector);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorHorizontalSum(FFloat128 Vector) noexcept
    {
        FFloat128 ShuffledVector = VectorShuffle<1, 0, 3, 2>(Vector);
        FFloat128 VectorSum      = VectorAdd(Vector, ShuffledVector);

        ShuffledVector = VectorShuffle0011<2, 3, 2, 3>(ShuffledVector, VectorSum);
        VectorSum      = VectorAdd(ShuffledVector, VectorSum);

        return VectorBroadcast<0>(VectorSum);
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

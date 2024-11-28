#pragma once
#include "Core/Math/MathCommon.h"

#ifndef VECTOR_ALIGN
    #define VECTOR_ALIGN ALIGN_AS(16)
#endif

#if PLATFORM_ARCHITECTURE_X86_X64 && PLATFORM_SUPPORT_SSE_INTRIN
    #define USE_VECTOR_MATH (1)

    #if PLATFORM_SUPPORT_SSE4_2_INTRIN
        #define USE_INT_VECTOR_MATH (1)

        #include "Core/Math/VectorMath/VectorMathSSE4_2.h"
        typedef FVectorMathSSE4_2 FPlatformVectorMath;
    #elif PLATFORM_SUPPORT_SSE4_1_INTRIN
        #define USE_INT_VECTOR_MATH (1)

        #include "Core/Math/VectorMath/VectorMathSSE4_1.h"
        typedef FVectorMathSSE4_1 FPlatformVectorMath;
    #elif PLATFORM_SUPPORT_SSSE3_INTRIN
        #define USE_INT_VECTOR_MATH (1)

        #include "Core/Math/VectorMath/VectorMathSSSE3.h"
        typedef FVectorMathSSSE3 FPlatformVectorMath;
    #elif PLATFORM_SUPPORT_SSE3_INTRIN
        #define USE_INT_VECTOR_MATH (1)

        #include "Core/Math/VectorMath/VectorMathSSE3.h"
        typedef FVectorMathSSE3 FPlatformVectorMath;
    #elif PLATFORM_SUPPORT_SSE2_INTRIN
        #define USE_INT_VECTOR_MATH (1)

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

    static FORCEINLINE FFloat128 VECTORCALL VectorMul(const float* VectorA, FFloat128 VectorB) noexcept
    {
        FFloat128 VectorA_128 = VectorLoad(VectorA);
        return VectorMul(VectorA_128, VectorB);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorMul(FFloat128 VectorA, const float* VectorB) noexcept
    {
        FFloat128 VectorB_128 = VectorLoad(VectorB);
        return VectorMul(VectorA, VectorB_128);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorMul(const float* VectorA, const float* VectorB) noexcept
    {
        FFloat128 VectorA_128 = VectorLoad(VectorA);
        FFloat128 VectorB_128 = VectorLoad(VectorB);
        return VectorMul(VectorA_128, VectorB_128);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorDiv(const float* VectorA, FFloat128 VectorB) noexcept
    {
        FFloat128 VectorA_128 = VectorLoad(VectorA);
        return VectorDiv(VectorA_128, VectorB);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorDiv(FFloat128 VectorA, const float* VectorB) noexcept
    {
        FFloat128 VectorB_128 = VectorLoad(VectorB);
        return VectorDiv(VectorA, VectorB_128);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorDiv(const float* VectorA, const float* VectorB) noexcept
    {
        FFloat128 VectorA_128 = VectorLoad(VectorA);
        FFloat128 VectorB_128 = VectorLoad(VectorB);
        return VectorDiv(VectorA_128, VectorB_128);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorAdd(const float* VectorA, FFloat128 VectorB) noexcept
    {
        FFloat128 VectorA_128 = VectorLoad(VectorA);
        return VectorAdd(VectorA_128, VectorB);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorAdd(FFloat128 VectorA, const float* VectorB) noexcept
    {
        FFloat128 VectorB_128 = VectorLoad(VectorB);
        return VectorAdd(VectorA, VectorB_128);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorAdd(const float* VectorA, const float* VectorB) noexcept
    {
        FFloat128 VectorA_128 = VectorLoad(VectorA);
        FFloat128 VectorB_128 = VectorLoad(VectorB);
        return VectorAdd(VectorA_128, VectorB_128);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSub(const float* VectorA, FFloat128 VectorB) noexcept
    {
        FFloat128 VectorA_128 = VectorLoad(VectorA);
        return VectorSub(VectorA_128, VectorB);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSub(FFloat128 VectorA, const float* VectorB) noexcept
    {
        FFloat128 VectorB_128 = VectorLoad(VectorB);
        return VectorSub(VectorA, VectorB_128);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSub(const float* VectorA, const float* VectorB) noexcept
    {
        FFloat128 VectorA_128 = VectorLoad(VectorA);
        FFloat128 VectorB_128 = VectorLoad(VectorB);
        return VectorSub(VectorA_128, VectorB_128);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorAbs(FFloat128 Vector) noexcept
    {
        static constexpr int32 Mask = ~(1 << 31);

        FInt128 Mask_128 = VectorSetInt1(Mask);
        return VectorAnd(Vector, VectorIntToFloat(Mask_128));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorCross(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        // Shuffle to get (Ay, Az, Ax, w) and (By, Bz, Bx, w)
        FFloat128 VectorC = VectorShuffle<1, 2, 0, 3>(VectorA);
        FFloat128 VectorD = VectorShuffle<1, 2, 0, 3>(VectorB);

        VectorC = VectorMul(VectorB, VectorC); // (Ax * By, Ay * Bz, Az * Bx, w)
        VectorD = VectorMul(VectorA, VectorD); // (Bx * Ay, By * Az, Bz * Ax, w)

        FFloat128 Result = VectorSub(VectorD, VectorC); // (Ay * Bz - Az * By, Az * Bx - Ax * Bz, Ax * By - Ay * Bx, 0)

        // Shuffle back to (X, Y, Z, w)
        return VectorShuffle<1, 2, 0, 3>(Result);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorTransform(const float* Matrix, FFloat128 Vector) noexcept
    {
        FFloat128 MatrixRow_128 = VectorLoad(&Matrix[0]);
        FFloat128 VectorA = VectorMul(VectorBroadcast<0>(Vector), MatrixRow_128);

        MatrixRow_128 = VectorLoad(&Matrix[4]);
        VectorA = VectorAdd(VectorA, VectorMul(VectorBroadcast<1>(Vector), MatrixRow_128));

        MatrixRow_128 = VectorLoad(&Matrix[8]);
        VectorA = VectorAdd(VectorA, VectorMul(VectorBroadcast<2>(Vector), MatrixRow_128));

        MatrixRow_128 = VectorLoad(&Matrix[12]);
        return VectorAdd(VectorA, VectorMul(VectorBroadcast<3>(Vector), MatrixRow_128));
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorHorizontalAdd(FFloat128 Vector) noexcept
    {
        return VectorHorizontalAdd(Vector, Vector);
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorClamp(FFloat128 Value, FFloat128 MinValue, FFloat128 MaxValue) noexcept
    {
        return VectorMin(MinValue, VectorMax(MaxValue, Value));
    }

    static FORCEINLINE FInt128 VECTORCALL VectorClampInt(FInt128 Value, FInt128 MinValue, FInt128 MaxValue) noexcept
    {
        return VectorMinInt(MinValue, VectorMaxInt(MaxValue, Value));
    }

    static FORCEINLINE FFloat128 VECTORCALL Matrix2Mul(FFloat128 MatrixA, FFloat128 MatrixB)
    {
        FFloat128 MatrixC = VectorShuffle<0, 3, 0, 3>(MatrixB);
        FFloat128 MatrixD = VectorMul(MatrixA, MatrixC);
        FFloat128 MatrixE = VectorShuffle<1, 0, 3, 2>(MatrixA);
        MatrixC = VectorShuffle<2, 1, 2, 1>(MatrixB);

        FFloat128 MatrixF = VectorMul(MatrixE, MatrixC);
        return VectorAdd(MatrixD, MatrixF);
    }

    static FORCEINLINE FFloat128 VECTORCALL Matrix2AdjointMul(FFloat128 MatrixA, FFloat128 MatrixB)
    {
        FFloat128 MatrixC = VectorShuffle<1, 1, 2, 2>(MatrixA);
        FFloat128 MatrixD = VectorShuffle<2, 3, 0, 1>(MatrixB);
        FFloat128 MatrixE = VectorMul(MatrixC, MatrixD);

        MatrixC = VectorShuffle<3, 3, 0, 0>(MatrixA);
        MatrixD = VectorMul(MatrixC, MatrixB);

        return VectorSub(MatrixD, MatrixE);
    }

    static FORCEINLINE FFloat128 VECTORCALL Matrix2MulAdjoint(FFloat128 MatrixA, FFloat128 MatrixB)
    {
        FFloat128 MatrixC = VectorShuffle<1, 0, 3, 2>(MatrixA);
        FFloat128 MatrixD = VectorShuffle<2, 1, 2, 1>(MatrixB);
        FFloat128 MatrixE = VectorMul(MatrixC, MatrixD);

        MatrixC = VectorShuffle<3, 0, 3, 0>(MatrixB);
        MatrixD = VectorMul(MatrixA, MatrixC);

        return VectorSub(MatrixD, MatrixE);
    }

    static FORCEINLINE void VECTORCALL Matrix4Transpose(const float* InMatrix, float* OutMatrix) noexcept
    {
        FFloat128 MatrixRowA_128 = VectorLoad(&InMatrix[0]);
        FFloat128 MatrixRowB_128 = VectorLoad(&InMatrix[4]);
        FFloat128 VectorA = VectorShuffle0101<0, 0, 1, 1>(MatrixRowA_128, MatrixRowB_128);
        FFloat128 VectorB = VectorShuffle0101<2, 2, 3, 3>(MatrixRowA_128, MatrixRowB_128);

        MatrixRowA_128 = VectorLoad(&InMatrix[8]);
        MatrixRowB_128 = VectorLoad(&InMatrix[12]);

        FFloat128 VectorC = VectorShuffle0101<0, 0, 1, 1>(MatrixRowA_128, MatrixRowB_128);
        FFloat128 VectorD = VectorShuffle0101<2, 2, 3, 3>(MatrixRowA_128, MatrixRowB_128);

        FFloat128 OutVector = VectorShuffle0011<0, 1, 0, 1>(VectorA, VectorC);
        VectorStore(OutVector, &OutMatrix[0]);

        OutVector = VectorShuffle0011<2, 3, 2, 3>(VectorA, VectorC);
        VectorStore(OutVector, &OutMatrix[4]);

        OutVector = VectorShuffle0011<0, 1, 0, 1>(VectorB, VectorD);
        VectorStore(OutVector, &OutMatrix[8]);

        OutVector = VectorShuffle0011<2, 3, 2, 3>(VectorB, VectorD);
        VectorStore(OutVector, &OutMatrix[12]);
    }
};

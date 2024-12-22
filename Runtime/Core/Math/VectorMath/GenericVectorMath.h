#pragma once
#include "Core/Math/MathCommon.h"

struct FInt128
{
    int32 x = 0;
    int32 y = 0;
    int32 z = 0;
    int32 w = 0;
};

struct FFloat128
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;
};

struct FGenericVectorMath
{
    static FORCEINLINE FFloat128 VECTORCALL VectorLoad(const float* Source) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSet(float x, float y, float z, float w) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSet1(float Scalar) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSetScalar(float Scalar) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorOne() noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorZero() noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE void VECTORCALL VectorStore(FFloat128 Vector, float* Dest) noexcept { }

    static FORCEINLINE void VECTORCALL VectorStore3(FFloat128 Vector, float* Dest) noexcept { }

    template<uint8 ComponentIndexX, uint8 ComponentIndexY, uint8 ComponentIndexZ, uint8 ComponentIndexW>
    static FORCEINLINE FFloat128 VECTORCALL VectorShuffle(FFloat128 VectorA) noexcept
    {
        return FFloat128();
    }

    template<uint8 ComponentIndexX, uint8 ComponentIndexY, uint8 ComponentIndexZ, uint8 ComponentIndexW>
    static FORCEINLINE FFloat128 VECTORCALL VectorShuffle0011(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128();
    }

    template<uint8 ComponentIndexX, uint8 ComponentIndexY, uint8 ComponentIndexZ, uint8 ComponentIndexW>
    static FORCEINLINE FFloat128 VECTORCALL VectorShuffle0101(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128();
    }

    template<uint8 ComponentIndex>
    static FORCEINLINE FFloat128 VECTORCALL VectorBroadcast(FFloat128 Vector) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE float VECTORCALL VectorGetX(FFloat128 Vector) noexcept
    {
        return 0.0f;
    }

    static FORCEINLINE float VECTORCALL VectorGetY(FFloat128 Vector) noexcept
    {
        return 0.0f;
    }

    static FORCEINLINE float VECTORCALL VectorGetZ(FFloat128 Vector) noexcept
    {
        return 0.0f;
    }

    static FORCEINLINE float VECTORCALL VectorGetW(FFloat128 Vector) noexcept
    {
        return 0.0f;
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorMul(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorDiv(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorAdd(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSub(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorHorizontalAdd(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorHorizontalSum(FFloat128 Vector) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSqrt(FFloat128 Vector) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorRecipSqrt(FFloat128 Vector) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorRecip(FFloat128 Vector) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorAnd(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorOr(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorDot(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE bool VECTORCALL VectorAllEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return 0.0f;
    }

    static FORCEINLINE bool VECTORCALL VectorAllGreaterThan(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return 0.0f;
    }

    static FORCEINLINE bool VECTORCALL VectorAllGreaterThanOrEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return 0.0f;
    }

    static FORCEINLINE bool VECTORCALL VectorAllLessThan(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return 0.0f;
    }

    static FORCEINLINE bool VECTORCALL VectorAllLessThanOrEqual(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return 0.0f;
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorMin(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorMax(FFloat128 VectorA, FFloat128 VectorB) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FInt128 VECTORCALL VectorLoadInt(const int32* Source) noexcept
    {
        return FInt128();
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSetInt(int32 x, int32 y, int32 z, int32 w) noexcept
    {
        return FInt128();
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSetInt1(int32 Scalar) noexcept
    {
        return FInt128();
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSetScalarInt(int32 Scalar) noexcept
    {
        return FInt128();
    }

    static FORCEINLINE FInt128 VECTORCALL VectorZeroInt() noexcept
    {
        return FInt128();
    }

    static FORCEINLINE FInt128 VECTORCALL VectorOneInt() noexcept
    {
        return FInt128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorIntToFloat(FInt128 Vector) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FInt128 VECTORCALL VectorFloatToInt(FFloat128 Vector) noexcept
    {
        return FInt128();
    }

    static FORCEINLINE void VECTORCALL VectorStoreInt(FInt128 Vector, int32* Dest) noexcept { }

    static FORCEINLINE void VECTORCALL VectorStoreInt3(FInt128 Vector, int32* Dest) noexcept { }

    static FORCEINLINE FInt128 VECTORCALL VectorAddInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return FInt128();
    }

    static FORCEINLINE FInt128 VECTORCALL VectorSubInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return FInt128();
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMulInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return FInt128();
    }

    static FORCEINLINE bool VECTORCALL VectorEqualInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return false;
    }

    static FORCEINLINE bool VECTORCALL VectorGreaterThanInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return false;
    }

    static FORCEINLINE bool VECTORCALL VectorGreaterThanOrEqualInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return false;
    }

    static FORCEINLINE bool VECTORCALL VectorLessThanInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return false;
    }

    static FORCEINLINE bool VECTORCALL VectorLessThanOrEqualInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return false;
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMinInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return FInt128();
    }

    static FORCEINLINE FInt128 VECTORCALL VectorMaxInt(FInt128 VectorA, FInt128 VectorB) noexcept
    {
        return FInt128();
    }
};

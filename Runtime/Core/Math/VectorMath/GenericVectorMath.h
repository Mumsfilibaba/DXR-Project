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
    static FORCEINLINE FFloat128 VECTORCALL LoadAligned(const float* Array) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL Load(float x, float y, float z, float w) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL Load(float Scalar) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL LoadSingle(float Single) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL LoadOnes() noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL LoadZeros() noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FInt128 VECTORCALL LoadAligned(const int32* Array) noexcept
    {
        return FInt128();
    }

    static FORCEINLINE FInt128 VECTORCALL Load(int32 x, int32 y, int32 z, int32 w) noexcept
    {
        return FInt128();
    }

    static FORCEINLINE FInt128 VECTORCALL Load(int32 Scalar) noexcept
    {
        return FInt128();
    }

    static FORCEINLINE FFloat128 VECTORCALL CastIntToFloat(FInt128 A) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FInt128 VECTORCALL CastFloatToInt(FFloat128 A) noexcept
    {
        return FInt128();
    }

    static FORCEINLINE void VECTORCALL StoreAligned(FFloat128 Register, float* Array) noexcept
    {
    }

    template<uint8 x, uint8 y, uint8 z, uint8 w>
    static FORCEINLINE FFloat128 VECTORCALL Shuffle(FFloat128 A) noexcept
    {
        return FFloat128();
    }

    template<uint8 x, uint8 y, uint8 z, uint8 w>
    static FORCEINLINE FFloat128 VECTORCALL Shuffle0011(FFloat128 A, FFloat128 B) noexcept
    {
        return FFloat128();
    }

    template<uint8 x, uint8 y, uint8 z, uint8 w>
    static FORCEINLINE FFloat128 VECTORCALL Shuffle0101(FFloat128 A, FFloat128 B) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE float VECTORCALL GetX(FFloat128 Register) noexcept
    {
        return 0.0f;
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorMul(FFloat128 A, FFloat128 B) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorDiv(FFloat128 A, FFloat128 B) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorAdd(FFloat128 A, FFloat128 B) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL HorizontalAdd(FFloat128 A, FFloat128 B) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSub(FFloat128 A, FFloat128 B) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL VectorSqrt(FFloat128 A) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL RecipSqrt(FFloat128 A) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL Recip(FFloat128 A) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL And(FFloat128 A, FFloat128 B) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL Or(FFloat128 A, FFloat128 B) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE bool VECTORCALL Equal(FFloat128 A, FFloat128 B) noexcept
    {
        return false;
    }

    static FORCEINLINE bool VECTORCALL GreaterThan(FFloat128 A, FFloat128 B) noexcept
    {
        return false;
    }

    static FORCEINLINE bool VECTORCALL GreaterThanOrEqual(FFloat128 A, FFloat128 B) noexcept
    {
        return false;
    }

    static FORCEINLINE bool VECTORCALL LessThan(FFloat128 A, FFloat128 B) noexcept
    {
        return false;
    }

    static FORCEINLINE bool VECTORCALL LessThanOrEqual(FFloat128 A, FFloat128 B) noexcept
    {
        return false;
    }

    static FORCEINLINE FFloat128 VECTORCALL Min(FFloat128 A, FFloat128 B) noexcept
    {
        return FFloat128();
    }

    static FORCEINLINE FFloat128 VECTORCALL Max(FFloat128 A, FFloat128 B) noexcept
    {
        return FFloat128();
    }
};

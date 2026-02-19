#pragma once
#include "Core/Core.h"
#include "Core/Memory/Memory.h"
#include <math.h>

struct FGenericPlatformMath
{
    static FORCEINLINE float Sqrt(float Value)
    {
        return ::sqrtf(Value);
    }

    static FORCEINLINE double Sqrt(double Value)
    {
        return ::sqrt(Value);
    }

    static FORCEINLINE bool IsNaN(float Value)
    {
        return ::isnan(Value) != 0;
    }

    static FORCEINLINE bool IsNaN(double Value)
    {
        return ::isnan(Value) != 0;
    }

    static FORCEINLINE bool IsInfinity(float Value)
    {
        return ::isinf(Value) != 0;
    }

    static FORCEINLINE bool IsInfinity(double Value)
    {
        return ::isinf(Value) != 0;
    }

    template<typename T>
    static constexpr FORCEINLINE T Abs(T Value)
    {
        return (Value < T(0)) ? static_cast<T>(-Value) : Value;
    }

    static FORCEINLINE float Round(float Value)
    {
        return ::roundf(Value);
    }

    static FORCEINLINE double Round(double Value)
    {
        return ::round(Value);
    }

    static FORCEINLINE int32 RoundToInt(float Value)
    {
        return static_cast<int32>(::roundf(Value));
    }

    static FORCEINLINE int32 RoundToInt(double Value)
    {
        return static_cast<int32>(::round(Value));
    }

    static FORCEINLINE float Floor(float Value)
    {
        return ::floorf(Value);
    }

    static FORCEINLINE double Floor(double Value)
    {
        return ::floor(Value);
    }

    static FORCEINLINE int32 FloorToInt(float Value)
    {
        return static_cast<int32>(::floorf(Value));
    }

    static FORCEINLINE int32 FloorToInt(double Value)
    {
        return static_cast<int32>(::floor(Value));
    }

    static FORCEINLINE float Ceil(float Value)
    {
        return ::ceilf(Value);
    }

    static FORCEINLINE double Ceil(double Value)
    {
        return ::ceil(Value);
    }

    static FORCEINLINE int32 CeilToInt(float Value)
    {
        return static_cast<int32>(::ceilf(Value));
    }

    static FORCEINLINE int32 CeilToInt(double Value)
    {
        return static_cast<int32>(::ceil(Value));
    }

    static FORCEINLINE float Exp(float Value)
    {
        return ::expf(Value);
    }

    static FORCEINLINE double Exp(double Value)
    {
        return ::exp(Value);
    }

    static FORCEINLINE float Log2(float Value)
    {
        return ::log2f(Value);
    }

    static FORCEINLINE double Log2(double Value)
    {
        return ::log2(Value);
    }

    static FORCEINLINE float Asin(float Value)
    {
        return ::asinf(Value);
    }

    static FORCEINLINE double Asin(double Value)
    {
        return ::asin(Value);
    }

    static FORCEINLINE float Acos(float Value)
    {
        return ::acosf(Value);
    }

    static FORCEINLINE double Acos(double Value)
    {
        return ::acos(Value);
    }

    static FORCEINLINE float Atan2(float Y, float X)
    {
        return ::atan2f(Y, X);
    }

    static FORCEINLINE double Atan2(double Y, double X)
    {
        return ::atan2(Y, X);
    }

    static FORCEINLINE float Sin(float Value)
    {
        return ::sinf(Value);
    }

    static FORCEINLINE double Sin(double Value)
    {
        return ::sin(Value);
    }

    static FORCEINLINE float Cos(float Value)
    {
        return ::cosf(Value);
    }

    static FORCEINLINE double Cos(double Value)
    {
        return ::cos(Value);
    }

    static FORCEINLINE float Tan(float Value)
    {
        return ::tanf(Value);
    }

    static FORCEINLINE double Tan(double Value)
    {
        return ::tan(Value);
    }

    static FORCEINLINE float FMod(float Value, float Divider)
    {
        return ::fmodf(Value, Divider);
    }

    static FORCEINLINE double FMod(double Value, double Divider)
    {
        return ::fmod(Value, Divider);
    }
};

template<>
FORCEINLINE float FGenericPlatformMath::Abs<float>(float Value)
{
    return ::fabsf(Value);
}

template<>
FORCEINLINE double FGenericPlatformMath::Abs<double>(double Value)
{
    return ::fabs(Value);
}

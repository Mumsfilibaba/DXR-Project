#pragma once
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/NumericLimits.h"

#include <algorithm>
#include <cmath>

struct FMath
{
    inline static constexpr double kPI        = 3.1415926535898;
    inline static constexpr double kE         = 2.7182818284590;
    inline static constexpr double kHalfPI    = kPI / 2.0;
    inline static constexpr double kTwoPI     = kPI * 2.0;
    inline static constexpr double kOneDegree = kPI / 180.0;

    inline static constexpr float kPI_f        = 3.141592653f;
    inline static constexpr float kE_f         = 2.718281828f;
    inline static constexpr float kHalfPI_f    = kPI_f / 2.0f;
    inline static constexpr float kTwoPI_f     = 2.0f * kPI_f;
    inline static constexpr float kOneDegree_f = kPI_f / 180.0f;

    inline static constexpr float kIsEqualEpsilon = 0.0005f;

    template<typename T>
    static FORCEINLINE T Sqrt(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return std::sqrt(Value);
    }

    template<typename T>
    static constexpr T DivideByMultiple(T Value, uint32 Alignment) requires(TIsInteger<T>::Value)
    {
        return static_cast<T>((Value + Alignment - 1) / Alignment);
    }

    template<typename T>
    static constexpr T AlignUp(T Value, T Alignment) requires(TIsInteger<T>::Value)
    {
        const T Mask = Alignment - 1;
        return ((Value + Mask) & (~Mask));
    }

    template<typename T>
    static constexpr T AlignDown(T Value, T Alignment) requires(TIsInteger<T>::Value)
    {
        const T Mask = Alignment - 1;
        return (Value & (~Mask));
    }

    template<typename T>
    static constexpr T Lerp(T First, T Second, T Factor) requires(TIsFloatingPoint<T>::Value)
    {
        return First + Factor * (Second - First);
    }

    template<typename T>
    static constexpr T Min(T First, T Second) requires(TIsArithmetic<T>::Value)
    {
        return (First <= Second) ? First : Second;
    }

    template<typename T>
    static constexpr T Max(T First, T Second) requires(TIsArithmetic<T>::Value)
    {
        return (First >= Second) ? First : Second;
    }

    template<typename T>
    static constexpr T Clamp(T Value, T MinValue, T MaxValue) requires(TIsArithmetic<T>::Value)
    {
        return Min(MaxValue, Max(MinValue, Value));
    }

    template<typename T>
    static constexpr T Saturate(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return Clamp(Value, T(0.0), T(1.0));
    }

    template<typename T>
    static constexpr T Abs(T Value) requires(TIsArithmetic<T>::Value)
    {
        return std::abs(Value);
    }

    template<typename T>
    static FORCEINLINE T Round(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return std::round(Value);
    }

    template<typename T>
    static FORCEINLINE int32 RoundToInt(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return static_cast<int32>(std::round(Value));
    }

    template<typename T>
    static constexpr T ToRadians(T Value)
    {
        return Value * static_cast<T>(kPI_f / 180.0f);
    }

    template<typename T>
    static constexpr T ToDegrees(T Value)
    {
        return Value * static_cast<T>(180.0f / kPI_f);
    }

    template<typename T>
    static FORCEINLINE T Log2(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return static_cast<T>(std::log2(Value));
    }

    template<typename T>
    static FORCEINLINE T Asin(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return static_cast<T>(std::asin(Value));
    }

    template<typename T>
    static FORCEINLINE T Acos(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return static_cast<T>(std::acos(Value));
    }

    template<typename T>
    static FORCEINLINE T Atan2(T y, T x) requires(TIsFloatingPoint<T>::Value)
    {
        return static_cast<T>(std::atan2(y, x));
    }

    template<typename T>
    static FORCEINLINE T Sin(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return static_cast<T>(std::sin(Value));
    }

    template<typename T>
    static FORCEINLINE T Cos(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return static_cast<T>(std::cos(Value));
    }

    template<typename T>
    static FORCEINLINE T Tan(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return static_cast<T>(std::tan(Value));
    }

    template<typename T>
    static FORCEINLINE T Ceil(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return static_cast<T>(std::ceil(Value));
    }

    template<typename T>
    static FORCEINLINE T Floor(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return static_cast<T>(std::floor(Value));
    }

    template<typename T>
    static FORCEINLINE bool IsNaN(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return std::isnan(Value);
    }

    template<typename T>
    static FORCEINLINE bool IsInfinity(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return std::isinf(Value);
    }

    static constexpr float CubicInterp(float P0, float P1, float P2, float P3, float T)
    {
        float A = P3 - P2 - P0 + P1;
        float B = P0 - P1 - A;
        float C = P2 - P0;
        float D = P1;
        return (A * T * T * T) + (B * T * T) + (C * T) + D;
    }

    static constexpr uint32 BytesToNum32BitConstants(uint32 Bytes)
    {
        return Bytes / 4;
    }

    template<const uint32 kBits>
    static constexpr uint64 MaxNum()
    {
        return (static_cast<uint64>(1) << kBits) - 1;
    }
};

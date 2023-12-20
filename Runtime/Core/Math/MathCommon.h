#pragma once
#include "Core/Core.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/NumericLimits.h"

#include <algorithm>
#include <cmath>

#if PLATFORM_WINDOWS
    #include <intrin.h>
    #include <smmintrin.h>
#elif PLATFORM_MACOS && PLATFORM_ARCHITECTURE_X86_X64
    #include <immintrin.h>
#else
    #error No platform defined
#endif

struct FMath
{
    inline static constexpr const double kPI            = 3.1415926535898;
    inline static constexpr const double kE             = 2.7182818284590;
    inline static constexpr const double kHalfPI        = kPI / 2.0f;
    inline static constexpr const double kTwoPI         = kPI * 2.0;
    inline static constexpr const double kOneDegree     = kPI / 180.0f;

    inline static constexpr const float kPI_f           = 3.141592653f;
    inline static constexpr const float kE_f            = 2.718281828f;
    inline static constexpr const float kHalfPI_f       = kPI_f / 2.0f;
    inline static constexpr const float kTwoPI_f        = 2.0f * kPI_f;
    inline static constexpr const float kOneDegree_f    = kPI_f / 180.0f;

    inline static constexpr const float kIsEqualEpsilon = 0.0005f;

    static FORCEINLINE float VECTORCALL Sqrt(float Value)
    {
#if PLATFORM_ARCHITECTURE_X86_X64
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_load_ss(&Value)));
#else
        return std::sqrtf(Value);
#endif
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
        return ((Value) & (~Mask));
    }

    template<typename T>
    static constexpr T Lerp(T First, T Second, T Factor) requires(TIsFloatingPoint<T>::Value)
    {
        return (-Factor * Second) + ((First * Factor) + Second);
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
    static constexpr T Clamp(T ValueMin, T ValueMax, T Value) requires(TIsArithmetic<T>::Value)
    {
        return Min(ValueMax, Max(ValueMin, Value));
    }

    template<typename T>
    static constexpr T Abs(T Value) requires(TIsArithmetic<T>::Value)
    {
        if constexpr (TIsInteger<T>::Value)
        {
            if constexpr (TIsSigned<T>::Value)
            {
                T Mask = Value >> (TNumericLimits<T>::Digits() - 1);
                return (Value + Mask) ^ Mask;
            }
            else
            {
                return Value;
            }
        }
        else
        {
            return (Value >= 0) ? Value : -Value;
        }
    }

    template<typename T>
    static FORCEINLINE T Round(T a)
    {
        return static_cast<T>(std::roundf(static_cast<float>(a)));
    }

    template<typename T>
    static constexpr T ToRadians(T Degrees)
    {
        return static_cast<T>(static_cast<float>(Degrees) * (kPI_f / 180.0f));
    }

    template<typename T>
    static constexpr T ToDegrees(T Radians)
    {
        return static_cast<T>(static_cast<float>(Radians) * (180.0f / kPI_f));
    }

    template<typename T>
    static FORCEINLINE T Log2(T Value)
    {
        return static_cast<T>(std::log2(static_cast<double>(Value)));
    }

    template<typename T>
    static FORCEINLINE T Asin(T Value)
    {
        return static_cast<T>(std::asinf(static_cast<float>(Value)));
    }

    template<typename T>
    static FORCEINLINE T Atan2(T y, T x)
    {
        return static_cast<T>(std::atan2f(static_cast<float>(y), static_cast<float>(x)));
    }

    template<typename T>
    static FORCEINLINE T Sin(T Value)
    {
        return static_cast<T>(std::sinf(static_cast<float>(Value)));
    }

    template<typename T>
    static FORCEINLINE T Cos(T Value)
    {
        return static_cast<T>(std::cosf(static_cast<float>(Value)));
    }

    template<typename T>
    static FORCEINLINE T Tan(T Value)
    {
        return static_cast<T>(std::tanf(static_cast<float>(Value)));
    }

    template<typename T>
    static FORCEINLINE T Ceil(T Value)
    {
        return static_cast<T>(std::ceilf(static_cast<float>(Value)));
    }

    template<typename T>
    static FORCEINLINE typename TEnableIf<TIsFloatingPoint<T>::Value, bool>::Type IsNaN(T Float)
    {
        return isnan(Float);
    }

    template<typename T>
    static FORCEINLINE typename TEnableIf<TIsFloatingPoint<T>::Value, bool>::Type IsInfinity(T Float)
    {
        return isinf(Float);
    }

    static constexpr uint32 BytesToNum32BitConstants(uint32 Bytes)
    {
        return Bytes / 4;
    }

    template<const uint32 kBits>
    static constexpr uint32 MaxNum()
    {
        return ((1 << kBits) - 1);
    }
};

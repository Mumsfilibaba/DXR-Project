#pragma once
#include "Core/CoreDefines.h"
#include "Core/CoreTypes.h"

#include "Core/Templates/IsInteger.h"
#include "Core/Templates/EnableIf.h"
#include "Core/Templates/IsFloatingPoint.h"

#include <algorithm>
#include <cmath>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Windows specific
#if PLATFORM_WINDOWS
#include <intrin.h>
#include <smmintrin.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// MacOS specific
#elif PLATFORM_MACOS
#include <immintrin.h>
#else

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// No defined platform
#error No platform defined
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Math namespace

namespace NMath
{
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Constants

    constexpr const double kPI         = 3.1415926535898;
    constexpr const double kE          = 2.7182818284590;
    constexpr const double kHalfPI     = kPI / 2.0f;
    constexpr const double kTwoPI      = kPI * 2.0;
    constexpr const double kOneDegree  = kPI / 180.0f;

    constexpr const float kPI_f        = 3.141592653f;
    constexpr const float kE_f         = 2.718281828f;
    constexpr const float kHalfPI_f    = kPI_f / 2.0f;
    constexpr const float kTwoPI_f     = 2.0f * kPI_f;
    constexpr const float kOneDegree_f = kPI_f / 180.0f;

    constexpr const float kIsEqualEpsilon = 0.0005f;

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Functions

    FORCEINLINE float VECTORCALL Sqrt(float v)
    {
#if ARCHITECTURE_X86_X64
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_load_ss(&v)));
#else
        return sqrtf(v);
#endif
    }

    template<typename T>
    FORCEINLINE T DivideByMultiple(T v, uint32 Alignment)
    {
        static_assert(TIsInteger<T>::Value);
        return static_cast<T>((v + Alignment - 1) / Alignment);
    }

    template<typename T>
    FORCEINLINE T AlignUp(T v, T Alignment)
    {
        static_assert(TIsInteger<T>::Value);

        const T Mask = Alignment - 1;
        return ((v + Mask) & (~Mask));
    }

    template<typename T>
    FORCEINLINE T AlignDown(T v, T Alignment)
    {
        static_assert(TIsInteger<T>::Value);

        const T Mask = Alignment - 1;
        return ((v) & (~Mask));
    }

    FORCEINLINE float Lerp(float a, float b, float f)
    {
        return (-f * b) + ((a * f) + b);
    }

    template<typename T>
    FORCEINLINE T Min(T a, T b)
    {
        return (a <= b) ? a : b;
    }

    template<typename T>
    FORCEINLINE T Max(T a, T b)
    {
        return (a >= b) ? a : b;
    }

    template<typename T>
    FORCEINLINE T Clamp(T InMin, T InMax, T x)
    {
        return Min(InMax, Max(InMin, x));
    }

    template<typename T>
    FORCEINLINE T Abs(T a)
    {
        return ::abs(a);
        // return (a > T( 0 )) ? ((a * a) / a) : T( 0 ); // TODO: Causes crash?
    }

    template<typename T>
    FORCEINLINE T ToRadians(T Degrees)
    {
        return static_cast<T>(static_cast<float>(Degrees) * (kPI_f / 180.0f));
    }

    template<typename T>
    FORCEINLINE T ToDegrees(T Radians)
    {
        return static_cast<T>(static_cast<float>(Radians) * (180.0f / kPI_f));
    }

    template<typename T>
    FORCEINLINE T Log2(T x)
    {
        return static_cast<T>(std::log2((double)x));
    }

    template<typename T>
    FORCEINLINE T Asin(T v)
    {
        return static_cast<T>(std::asinf(static_cast<float>(v)));
    }

    template<typename T>
    FORCEINLINE T Atan2(T y, T x)
    {
        return static_cast<T>(std::atan2f(static_cast<float>(y), static_cast<float>(x)));
    }

    template<typename T>
    FORCEINLINE typename TEnableIf<TIsFloatingPoint<T>::Value, bool>::Type IsNaN(T Float)
    {
        return isnan(Float);
    }

    template<typename T>
    FORCEINLINE typename TEnableIf<TIsFloatingPoint<T>::Value, bool>::Type IsInfinity(T Float)
    {
        return isinf(Float);
    }

    FORCEINLINE uint32 BytesToNum32BitConstants(uint32 Bytes)
    {
        return Bytes / 4;
    }
}

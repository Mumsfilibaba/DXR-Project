#pragma once
#include "CoreDefines.h"
#include "CoreTypes.h"

#include <algorithm>
#include <cmath>

/* Windows specific */
#if defined(PLATFORM_WINDOWS)
#include <xmmintrin.h>
#else
#error No platform defined
#endif

namespace NMath
{
    constexpr const double PI = 3.1415926535898;
    constexpr const double E = 2.7182818284590;
    constexpr const double HALF_PI = PI / 2.0f;
    constexpr const double TWO_PI = PI * 2.0;
    constexpr const double ONE_DEGREE = PI / 180.0f;

    constexpr const float PI_F = 3.141592653f;
    constexpr const float E_F = 2.718281828f;
    constexpr const float HALF_PI_F = PI_F / 2.0f;
    constexpr const float TWO_PI_F = 2.0f * PI_F;
    constexpr const float ONE_DEGREE_F = PI_F / 180.0f;

    constexpr const float IS_EQUAL_EPISILON = 0.0005f;

    // TODO: Move all math functions and constants to one location
    FORCEINLINE float VECTORCALL Sqrt( float v )
    {
    #if ARCH_X86_X64
        return _mm_cvtss_f32( _mm_sqrt_ss( _mm_load_ss( &v ) ) );
    #else
        return sqrtf( v );
    #endif
    }

    template <typename T>
    FORCEINLINE T DivideByMultiple( T v, uint32 Alignment )
    {
        static_assert(std::is_integral<T>());

        return static_cast<T>((v + Alignment - 1) / Alignment);
    }

    template <typename T>
    FORCEINLINE T AlignUp( T v, T Alignment )
    {
        static_assert(std::is_integral<T>());

        const T Mask = Alignment - 1;
        return ((v + Mask) & (~Mask));
    }

    template <typename T>
    FORCEINLINE T AlignDown( T v, T Alignment )
    {
        static_assert(std::is_integral<T>());

        const T Mask = Alignment - 1;
        return ((v) & (~Mask));
    }

    FORCEINLINE float Lerp( float a, float b, float f )
    {
        return (-f * b) + ((a * f) + b);
    }

    template <typename T>
    FORCEINLINE T Min( T a, T b )
    {
        return a <= b ? a : b;
    }

    template <typename T>
    FORCEINLINE T Max( T a, T b )
    {
        return a >= b ? a : b;
    }

    template <typename T>
    FORCEINLINE T Abs( T a )
    {
        return (a * a) / a;
    }

    template <typename T>
    FORCEINLINE T ToRadians( T Degrees )
    {
        return static_cast<T>(static_cast<float>(Degrees) * (PI_F / 180.0f));
    }

    template <typename T>
    FORCEINLINE T ToDegrees( T Radians )
    {
        return static_cast<T>(static_cast<float>(Radians) * (180.0f / PI_F));
    }

    template <typename T>
    FORCEINLINE T Log2( T x )
    {
        return static_cast<T>(std::log2( (double)x ));
    }

    template <typename T>
    FORCEINLINE T Asin( T v )
    {
        return static_cast<T>(std::asinf( static_cast<float>(v) ));
    }

    template <typename T>
    FORCEINLINE T Atan2( T y, T x )
    {
        return static_cast<T>(std::atan2f( static_cast<float>(y), static_cast<float>(x) ));
    }

    FORCEINLINE uint32 BytesToNum32BitConstants( uint32 Bytes )
    {
        return Bytes / 4;
    }
}
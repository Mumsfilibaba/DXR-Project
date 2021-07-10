#pragma once
#include "Core/Types.h"
#include "MathDefines.h"
#include "Memory.h"

#include <algorithm>
#include <cmath>

/* Windows specific */
#if defined(_WIN32)

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
    FORCEINLINE float VECTORCALL Sqrt( float V )
    {
    #if X86_X64
        return _mm_cvtss_f32( _mm_sqrt_ss( _mm_load_ss( &V ) ) );
    #else
        return sqrtf( V );
    #endif
    }
}
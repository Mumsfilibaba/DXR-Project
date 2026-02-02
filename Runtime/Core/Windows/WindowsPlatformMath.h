#pragma once
#include "Core/Generic/GenericPlatformMath.h"

#include <float.h>
#include <xmmintrin.h>
#include <emmintrin.h>

struct FWindowsPlatformMath : public FGenericPlatformMath
{
    static FORCEINLINE bool IsNaN(float Value)
    {
        return _isnan(Value) != 0;
    }

    static FORCEINLINE bool IsNaN(double Value)
    {
        return _isnan(Value) != 0;
    }

    static FORCEINLINE bool IsInfinity(float Value)
    {
        // _finite == 0 for NaN and Inf; filter out NaN.
        return (_finite(Value) == 0) && (_isnan(Value) == 0);
    }

    static FORCEINLINE bool IsInfinity(double Value)
    {
        return (_finite(Value) == 0) && (_isnan(Value) == 0);
    }

    static FORCEINLINE float Sqrt(float Value)
    {
        const __m128 X = _mm_set_ss(Value);
        const __m128 R = _mm_sqrt_ss(X);
        return _mm_cvtss_f32(R);
    }

    static FORCEINLINE double Sqrt(double Value)
    {
        const __m128d X = _mm_set_sd(Value);
        const __m128d R = _mm_sqrt_sd(X, X);
        return _mm_cvtsd_f64(R);
    }
};

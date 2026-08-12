#pragma once
#include "Core/PlatformInterface/IPlatformMath.h"

struct FMacPlatformMath : public IPlatformMath
{
    static FORCEINLINE bool IsNaN(float Value)
    {
        return __builtin_isnan(Value) != 0;
    }

    static FORCEINLINE bool IsNaN(double Value)
    {
        return __builtin_isnan(Value) != 0;
    }

    static FORCEINLINE bool IsInfinity(float Value)
    {
        return __builtin_isinf(Value) != 0;
    }

    static FORCEINLINE bool IsInfinity(double Value)
    {
        return __builtin_isinf(Value) != 0;
    }

    static FORCEINLINE float Sqrt(float Value)
    {
        return __builtin_sqrtf(Value);
    }

    static FORCEINLINE double Sqrt(double Value)
    {
        return __builtin_sqrt(Value);
    }
};

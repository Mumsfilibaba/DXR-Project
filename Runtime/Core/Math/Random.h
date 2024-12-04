#pragma once
#include "Core/Math/MathCommon.h"
#include "Core/Templates/NumericLimits.h"
#include "Core/Platform/PlatformTime.h"

class FRandom 
{
    static constexpr uint64 Multipler = 0x2545F4914F6CDD1D;

public:
    FORCEINLINE FRandom() 
        : Seed(FPlatformTime::QueryPerformanceCounter()) 
    {
    }

    FORCEINLINE FRandom(uint32 InSeed)
        : Seed(InSeed)
    {
    }

    FORCEINLINE uint64 Rand()
    {
        // Perform an XOR Shift
        Seed ^= Seed << 12;
        Seed ^= Seed >> 25;
        Seed ^= Seed << 27;
        return Seed * Multipler;
    }

    FORCEINLINE int64 RandInt(int64 Min, int64 Max)
    {
        const uint64 RandVal = Rand();
        const uint64 Range   = static_cast<uint64>(Max - Min) + 1;
        const uint64 Scaled  = RandVal % Range;
        const int64  Result  = static_cast<int64>(Scaled) + Min;
        return Result;
    }

    FORCEINLINE float RandFloat(float Min, float Max)
    {
        const uint64 RandVal = Rand();
        const float Fraction = static_cast<float>(RandVal) / static_cast<float>(TNumericLimits<uint64>::Max());
        const float Result   = Fraction * (Max - Min) + Min;
        return Result;
    }

private:
    uint64 Seed;
};
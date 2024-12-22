#pragma once
#include "Core/Math/MathCommon.h"
#include "Core/Templates/NumericLimits.h"
#include "Core/Platform/PlatformTime.h"

/**
 * @brief A pseudo-random number generator class. Uses an XOR shift algorithm for generating random numbers.
 */

class FRandom 
{
    static constexpr uint64 Multiplier = 0x2545F4914F6CDD1D;

public:

    /**
     * @brief Default constructor.
     * Initializes the random number generator with the current performance counter.
     */
    FORCEINLINE FRandom() 
        : Seed(FPlatformTime::QueryPerformanceCounter()) 
    {
    }

    /**
     * @brief Constructor with a custom seed.
     * @param InSeed The seed value to initialize the random number generator.
     */
    FORCEINLINE FRandom(uint32 InSeed)
        : Seed(InSeed)
    {
    }

    /**
     * @brief Generates a random 64-bit unsigned integer.
     * @return A pseudo-random 64-bit unsigned integer.
     */
    FORCEINLINE uint64 Rand()
    {
        // Perform an XOR Shift
        Seed ^= Seed << 12;
        Seed ^= Seed >> 25;
        Seed ^= Seed << 27;
        return Seed * Multiplier;
    }

    /**
     * @brief Generates a random integer within a specified range.
     * @param Min The minimum value of the range (inclusive).
     * @param Max The maximum value of the range (inclusive).
     * @return A random integer within [Min, Max].
     */
    FORCEINLINE int64 RandInt(int64 Min, int64 Max)
    {
        const uint64 RandVal = Rand();
        const uint64 Range   = static_cast<uint64>(Max - Min) + 1;
        const uint64 Scaled  = RandVal % Range;
        const int64  Result  = static_cast<int64>(Scaled) + Min;
        return Result;
    }

    /**
     * @brief Generates a random floating-point number within a specified range.
     * @param Min The minimum value of the range.
     * @param Max The maximum value of the range.
     * @return A random floating-point number within [Min, Max].
     */
    FORCEINLINE float RandFloat(float Min, float Max)
    {
        const uint64 RandVal = Rand();
        const float Fraction = static_cast<float>(RandVal) / static_cast<float>(TNumericLimits<uint64>::Max());
        const float Result   = Fraction * (Max - Min) + Min;
        return Result;
    }

    /**
     * @brief Sets a new seed for the random number generator.
     * @param NewSeed The new seed value.
     */
    FORCEINLINE void SetSeed(uint64 NewSeed)
    {
        Seed = NewSeed;
    }

    /**
     * @brief Generates a random boolean value.
     * @return True or False randomly.
     */
    FORCEINLINE bool RandBool()
    {
        return (Rand() & 1) == 1;
    }

private:

    /** @brief The internal seed value for the random number generator. */
    uint64 Seed;
};

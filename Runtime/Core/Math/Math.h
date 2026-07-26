#pragma once
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/NumericLimits.h"
#include "Core/Platform/PlatformMath.h"

struct Math : public FPlatformMath
{
    // -------------------------------------------------------------------------------------------
    // Constants
    // -------------------------------------------------------------------------------------------

    struct Constants
    {
        // -------------------------------------------------------------------------------------------
        // Fundamental Constants
        // -------------------------------------------------------------------------------------------

        static constexpr float PI        = 3.14159265358979323846f;   // π
        static constexpr float E         = 2.71828182845904523536f;   // Euler’s number
        static constexpr float HalfPI    = PI * 0.5f;                 // π / 2
        static constexpr float TwoPI     = PI * 2.0f;                 // 2π
        static constexpr float QuarterPI = PI * 0.25f;                // π / 4

        // -------------------------------------------------------------------------------------------
        // Degree / Radian Conversions
        // -------------------------------------------------------------------------------------------

        static constexpr float RadToDeg = 180.0f / PI; // Radians → degrees
        static constexpr float Deg2Rad  = PI / 180.0f; // Degrees → radians

        // -------------------------------------------------------------------------------------------
        // Inverse Constants
        // -------------------------------------------------------------------------------------------

        static constexpr float InvPI    = 1.0f / PI;    // 1/π
        static constexpr float InvTwoPI = 1.0f / TwoPI; // 1/(2π)

        // -------------------------------------------------------------------------------------------
        // Root Constants
        // -------------------------------------------------------------------------------------------

        static constexpr float Sqrt2     = 1.41421356237f; // √2
        static constexpr float Sqrt3     = 1.73205080757f; // √3
        static constexpr float SqrtPI    = 1.77245385091f; // √π
        static constexpr float InvSqrtPI = 0.56418958355f; // 1/√π
        static constexpr float InvSqrt2  = 0.70710678118f; // 1/√2

        // -------------------------------------------------------------------------------------------
        // Logarithmic / Exponential Constants
        // -------------------------------------------------------------------------------------------

        static constexpr float Ln2    = 0.69314718056f; // ln(2)
        static constexpr float Ln10   = 2.30258509300f; // ln(10)
        static constexpr float Log2E  = 1.44269504089f; // log₂(e)
        static constexpr float Log10E = 0.43429448190f; // log₁₀(e)

        // -------------------------------------------------------------------------------------------
        // Miscellaneous Constants
        // -------------------------------------------------------------------------------------------

        static constexpr float GoldenRatio = 1.61803398875f; // φ (golden ratio)

        // -------------------------------------------------------------------------------------------
        // Tolerances / Numeric Limits
        // -------------------------------------------------------------------------------------------

        static constexpr float Epsilon      = TNumericLimits<float>::Epsilon();  // FLT_EPSILON
        static constexpr float Infinity     = TNumericLimits<float>::Infinity(); // Positive infinity sentinel
        static constexpr float NaN          = TNumericLimits<float>::NaN();      // Quiet NaN sentinel
        static constexpr float CmpThreshold = 5.0e-4f;                           // Threshold for float comparisons
    };

public:

    // -------------------------------------------------------------------------------------------
    // Standard Math (roots, rounding, trig, log)
    // -------------------------------------------------------------------------------------------

    using FPlatformMath::Sqrt;
    using FPlatformMath::Abs;
    using FPlatformMath::Round;
    using FPlatformMath::RoundToInt;
    using FPlatformMath::Floor;
    using FPlatformMath::FloorToInt;
    using FPlatformMath::Ceil;
    using FPlatformMath::CeilToInt;
    using FPlatformMath::Exp;
    using FPlatformMath::Log2;
    using FPlatformMath::Asin;
    using FPlatformMath::Acos;
    using FPlatformMath::Atan2;
    using FPlatformMath::Sin;
    using FPlatformMath::Cos;
    using FPlatformMath::Tan;
    using FPlatformMath::FMod;
    using FPlatformMath::IsNaN;
    using FPlatformMath::IsInfinity;

public:

    // -------------------------------------------------------------------------------------------
    // Alignment & Powers of Two
    // -------------------------------------------------------------------------------------------

    /** @brief Divides and rounds up to the nearest multiple of the given alignment. */
    template<typename T>
    static FORCEINLINE constexpr T DivideByMultiple(T Value, uint32 Alignment) requires(TIsInteger<T>::Value)
    {
        return static_cast<T>((Value + Alignment - T(1)) / Alignment);
    }

    /** @brief Rounds an integer up to the nearest aligned value. */
    template<typename T>
    static FORCEINLINE constexpr T AlignUp(T Value, T Alignment) requires(TIsInteger<T>::Value)
    {
        const T Mask = Alignment - 1;
        return ((Value + Mask) & (~Mask));
    }

    /** @brief Rounds an integer down to the nearest aligned value. */
    template<typename T>
    static FORCEINLINE constexpr T AlignDown(T Value, T Alignment) requires(TIsInteger<T>::Value)
    {
        const T Mask = Alignment - 1;
        return (Value & (~Mask));
    }

    /** @brief Rounds an integer up to the nearest multiple of the alignment. Supports non-power-of-two alignments. */
    template<typename T>
    static FORCEINLINE constexpr T AlignUpToMultiple(T Value, T Alignment) requires(TIsInteger<T>::Value)
    {
        return ((Value + Alignment - T(1)) / Alignment) * Alignment;
    }

    /** @brief Returns the greatest common divisor of two integers using Euclid's algorithm. */
    template<typename T>
    static FORCEINLINE constexpr T GreatestCommonDivisor(T ValueA, T ValueB) requires(TIsInteger<T>::Value)
    {
        while (ValueB != T(0))
        {
            const T Temp = ValueB;
            ValueB = ValueA % ValueB;
            ValueA = Temp;
        }

        return ValueA;
    }

    /** @brief Returns the least common multiple of two integers. */
    template<typename T>
    static FORCEINLINE constexpr T LeastCommonMultiple(T ValueA, T ValueB) requires(TIsInteger<T>::Value)
    {
        if (ValueA == T(0) || ValueB == T(0))
        {
            return T(0);
        }

        return (ValueA / GreatestCommonDivisor(ValueA, ValueB)) * ValueB;
    }

    /** @brief Returns true if the given integer is a power of two. */
    static FORCEINLINE constexpr bool IsPowerOfTwo(uint32 Value)
    {
        return Value && ((Value & (Value - 1)) == 0);
    }

    /** @brief Returns the power of two closest to the given integer. */
    static constexpr uint32 ClosestPowerOfTwo(uint32 Value)
    {
        if (Value == 0)
        {
            return 1;
        }

        if (IsPowerOfTwo(Value))
        {
            return Value;
        }

        uint32 UpperPower = 1;
        while (UpperPower < Value)
        {
            UpperPower <<= 1;
        }

        const uint32 LowerPower = UpperPower >> 1;
        return (Value - LowerPower < UpperPower - Value) ? LowerPower : UpperPower;
    }

    /** @brief Returns the next power of two strictly greater than the given integer. */
    static constexpr uint32 NextPowerOfTwo(uint32 Value)
    {
        if (Value == 0)
        {
            return 1;
        }

        uint32 Candidate = 1;
        while (Candidate <= Value)
        {
            Candidate <<= 1;
        }

        return Candidate;
    }

public:

    // -------------------------------------------------------------------------------------------
    // Interpolation
    // -------------------------------------------------------------------------------------------

    /** @brief Performs linear interpolation between two values. */
    template<typename T>
    static FORCEINLINE constexpr T Lerp(T First, T Second, T Factor) requires(TIsFloatingPoint<T>::Value)
    {
        return First + Factor * (Second - First);
    }

    /** @brief Performs cubic interpolation between four control points. */
    static FORCEINLINE constexpr float CubicInterp(float P0, float P1, float P2, float P3, float T)
    {
        const float A = P3 - P2 - P0 + P1;
        const float B = P0 - P1 - A;
        const float C = P2 - P0;
        const float D = P1;
        return (A * T * T * T) + (B * T * T) + (C * T) + D;
    }

public:

    // -------------------------------------------------------------------------------------------
    // Min / Max / Clamp / Swap
    // -------------------------------------------------------------------------------------------

    /** @brief Returns the smaller of two values. */
    template<typename T>
    static FORCEINLINE constexpr T Min(T First, T Second) requires(TIsArithmetic<T>::Value)
    {
        return (First <= Second) ? First : Second;
    }

    /** @brief Returns the larger of two values. */
    template<typename T>
    static FORCEINLINE constexpr T Max(T First, T Second) requires(TIsArithmetic<T>::Value)
    {
        return (First >= Second) ? First : Second;
    }

    /** @brief Clamps a value between a minimum and maximum range. */
    template<typename T>
    static FORCEINLINE constexpr T Clamp(T Value, T MinValue, T MaxValue) requires(TIsArithmetic<T>::Value)
    {
        return Min(MaxValue, Max(MinValue, Value));
    }

    /** @brief Clamps a float between 0.0 and 1.0. */
    template<typename T>
    static FORCEINLINE constexpr T Saturate(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return Clamp(Value, T(0.0), T(1.0));
    }

    /** @brief Swaps the contents of two variables. */
    template<typename T>
    static FORCEINLINE void Swap(T& ValueA, T& ValueB) noexcept
    {
        if (AddressOf(ValueA) == AddressOf(ValueB))
        {
            return;
        }

        T Temp = Move(ValueA);
        ValueA = Move(ValueB);
        ValueB = Move(Temp);
    }

public:

    // -------------------------------------------------------------------------------------------
    // Conversions
    // -------------------------------------------------------------------------------------------
    
    /** @brief Converts degrees to radians. */
    template<typename T>
    static FORCEINLINE constexpr T DegreesToRadians(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return Value * static_cast<T>(Constants::PI / 180.0);
    }

    /** @brief Converts radians to degrees. */
    template<typename T>
    static FORCEINLINE constexpr T RadiansToDegrees(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return Value * static_cast<T>(180.0 / Constants::PI);
    }

public:

    // -------------------------------------------------------------------------------------------
    // Bit / Numeric Helpers
    // -------------------------------------------------------------------------------------------

    /** @brief Reinterprets a float as its uint32 bit pattern (avoids aliasing UB). */
    static FORCEINLINE uint32 FloatToBits(float Value) noexcept
    {
        static_assert(sizeof(float) == sizeof(uint32));
        uint32 Bits = 0;
        Memory::Memcpy(&Bits, &Value, sizeof(Bits));
        return Bits;
    }

    /** @brief Reinterprets a uint32 bit pattern as a float (avoids aliasing UB). */
    static FORCEINLINE float BitsToFloat(uint32 Bits) noexcept
    {
        float Value = 0.0f;
        Memory::Memcpy(&Value, &Bits, sizeof(Value));
        return Value;
    }

    /** @brief Reinterprets an int32 as its uint32 bit pattern (avoids aliasing UB). */
    static FORCEINLINE uint32 IntToBits(int32 Value) noexcept
    {
        static_assert(sizeof(int32) == sizeof(uint32));
        uint32 Bits = 0;
        Memory::Memcpy(&Bits, &Value, sizeof(Bits));
        return Bits;
    }

    /** @brief Reinterprets a uint32 bit pattern as an int32 (avoids aliasing UB). */
    static FORCEINLINE int32 BitsToInt(uint32 Bits) noexcept
    {
        int32 Value = 0;
        Memory::Memcpy(&Value, &Bits, sizeof(Value));
        return Value;
    }

    /** @brief Adds two int32 values with wrap-around semantics (avoids signed overflow UB). */
    static FORCEINLINE int32 AddWrapInt32(int32 A, int32 B) noexcept
    {
        const uint32 UA = IntToBits(A);
        const uint32 UB = IntToBits(B);
        return BitsToInt(UA + UB);
    }

    /** @brief Subtracts two int32 values with wrap-around semantics (avoids signed overflow UB). */
    static FORCEINLINE int32 SubWrapInt32(int32 A, int32 B) noexcept
    {
        const uint32 UA = IntToBits(A);
        const uint32 UB = IntToBits(B);
        return BitsToInt(UA - UB);
    }

    /** @brief Multiplies two int32 values with wrap-around semantics (avoids signed overflow UB). */
    static FORCEINLINE int32 MulWrapInt32(int32 A, int32 B) noexcept
    {
        const uint64 UA = static_cast<uint64>(IntToBits(A));
        const uint64 UB = static_cast<uint64>(IntToBits(B));
        const uint32 Lo = static_cast<uint32>(UA * UB);
        return BitsToInt(Lo);
    }

    /** @brief Returns 0xFFFFFFFF for true, otherwise 0. */
    static FORCEINLINE uint32 BoolMask(bool bValue) noexcept
    {
        return bValue ? 0xFFFFFFFFu : 0u;
    }

    /** @brief Returns the maximum representable value for the given bit width. */
    template<const uint64 NumBits>
    static FORCEINLINE constexpr uint64 MaxNum()
    {
        return (static_cast<uint64>(1) << NumBits) - 1;
    }

public:

    // -------------------------------------------------------------------------------------------
    // Texture / Mip Helpers
    // -------------------------------------------------------------------------------------------

    /** @brief Computes the number of mip levels for a single dimension. Returns floor(log2(dimension)) + 1 for dimension >= 1, else 0. */
    static FORCEINLINE uint32 MipCountFromDimension(uint32 Dimension)
    {
        if (Dimension == 0)
        {
            return 0;
        }
        
        return static_cast<uint32>(FloorToInt(Log2(static_cast<double>(Dimension)))) + 1u;
    }

    /** @brief Computes the maximum possible number of mip levels for an extent. If Depth == 0 (for 1D/2D textures), it is treated as 1. */
    static FORCEINLINE uint32 MaxMipLevelsFromExtent(uint32 Width, uint32 Height, uint32 Depth)
    {
        const uint32 DepthClamped = (Depth == 0u) ? 1u : Depth;
        const uint32 LargestDim   = Max(Width, Max(Height, DepthClamped));
        return MipCountFromDimension(LargestDim);
    }

    /** @brief Computes the number of mip levels where both Width and Height remain >= MinSize. Returns 0 if the base dimensions are already below MinSize. */
    static FORCEINLINE uint32 MipCountAboveMinSize(uint32 Width, uint32 Height, uint32 MinSize)
    {
        if (Width == 0 || Height == 0 || MinSize == 0)
        {
            return 0;
        }

        const uint32 SmallestDim = Min(Width, Height);
        if (SmallestDim < MinSize)
        {
            return 0;
        }

        return MipCountFromDimension(SmallestDim / MinSize);
    }
};

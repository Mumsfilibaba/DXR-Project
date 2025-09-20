#pragma once
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/NumericLimits.h"
#include <algorithm>
#include <cmath>

struct FMath
{
public:
    static constexpr double PI        = 3.1415926535898;
    static constexpr double E         = 2.7182818284590;
    static constexpr double HalfPI    = PI / 2.0;
    static constexpr double TwoPI     = PI * 2.0;
    static constexpr double OneDegree = PI / 180.0;

    static constexpr float PI_Float        = 3.141592653f;
    static constexpr float E_Float         = 2.718281828f;
    static constexpr float HalfPI_Float    = PI_Float / 2.0f;
    static constexpr float TwoPI_Float     = 2.0f * PI_Float;
    static constexpr float OneDegree_Float = PI_Float / 180.0f;

    static constexpr float FloatCompareEpsilon = 0.0005f;

public:
    
    // Standard Math

    template<typename T>
    static FORCEINLINE T Sqrt(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return std::sqrt(Value);
    }

    template<typename T>
    static FORCEINLINE constexpr T Abs(T Value) requires(TIsArithmetic<T>::Value)
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
    static FORCEINLINE T Floor(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return static_cast<T>(std::floor(Value));
    }

    template<typename T>
    static FORCEINLINE int32 FloorToInt(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return static_cast<int32>(std::floor(Value));
    }

    template<typename T>
    static FORCEINLINE T Ceil(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return static_cast<T>(std::ceil(Value));
    }

    template<typename T>
    static FORCEINLINE int32 CeilToInt(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return static_cast<int32>(std::ceil(Value));
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
    static FORCEINLINE T FMod(T Value, T Divider) requires(TIsFloatingPoint<T>::Value)
    {
        return static_cast<T>(std::fmod(Value, Divider));
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

public:
    
    // Alignment

    template<typename T>
    static FORCEINLINE constexpr T DivideByMultiple(T Value, uint32 Alignment) requires(TIsInteger<T>::Value)
    {
        return static_cast<T>((Value + Alignment - 1) / Alignment);
    }

    template<typename T>
    static FORCEINLINE constexpr T AlignUp(T Value, T Alignment) requires(TIsInteger<T>::Value)
    {
        const T Mask = Alignment - 1;
        return ((Value + Mask) & (~Mask));
    }

    template<typename T>
    static FORCEINLINE constexpr T AlignDown(T Value, T Alignment) requires(TIsInteger<T>::Value)
    {
        const T Mask = Alignment - 1;
        return (Value & (~Mask));
    }

    static FORCEINLINE constexpr bool IsPowerOfTwo(uint32 Value) 
    {
        return Value && ((Value & (Value - 1)) == 0);
    }

    static constexpr uint32 ClosestPowerOfTwo(uint32 Value)
    {
        // Handle the edge case where Value is 0
        if (Value == 0)
        {
            return 1;
        }
        
        // If already a power of two, return Value
        if (IsPowerOfTwo(Value))
        {
            return Value;
        }
        
        // Find the upper power of two (The power of two that comes after value)
        uint32 UpperPower = 1;
        while (UpperPower < Value)
        {
            UpperPower <<= 1;
        }
        
        // The previous power of two
        const uint32 LowerPower = UpperPower >> 1;
        
        // Compare differences: if Value is closer to LowerPower, return LowerPower; otherwise return UpperPower.
        if (Value - LowerPower < UpperPower - Value)
        {
            return LowerPower;
        }
        else
        {
            return UpperPower;
        }
    }

    static constexpr uint32 NextPowerOfTwo(uint32 Value)
    {
        // Handle the edge case where Value is 0
        if (Value == 0)
        {
            return 1;
        }

        // Shift Candidate until it is strictly greater than Value.
        uint32 Candidate = 1;
        while (Candidate <= Value)
        {
            Candidate <<= 1;
        }
        
        return Candidate;
    }

public: 
    
    // Interpolation

    template<typename T>
    static FORCEINLINE constexpr T Lerp(T First, T Second, T Factor) requires(TIsFloatingPoint<T>::Value)
    {
        return First + Factor * (Second - First);
    }

    static FORCEINLINE constexpr float CubicInterp(float P0, float P1, float P2, float P3, float T)
    {
        float A = P3 - P2 - P0 + P1;
        float B = P0 - P1 - A;
        float C = P2 - P0;
        float D = P1;
        return (A * T * T * T) + (B * T * T) + (C * T) + D;
    }

public: 
    
    // Min, Max, Clamp, ...

    template<typename T>
    static FORCEINLINE constexpr T Min(T First, T Second) requires(TIsArithmetic<T>::Value)
    {
        return (First <= Second) ? First : Second;
    }

    template<typename T>
    static FORCEINLINE constexpr T Max(T First, T Second) requires(TIsArithmetic<T>::Value)
    {
        return (First >= Second) ? First : Second;
    }

    template<typename T>
    static FORCEINLINE constexpr T Clamp(T Value, T MinValue, T MaxValue) requires(TIsArithmetic<T>::Value)
    {
        return Min(MaxValue, Max(MinValue, Value));
    }

    template<typename T>
    static FORCEINLINE constexpr T Saturate(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return Clamp(Value, T(0.0), T(1.0));
    }

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
    
    // Conversions

    template<typename T>
    static FORCEINLINE constexpr T DegreesToRadians(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return Value * static_cast<T>(PI / 180.0);
    }

    template<typename T>
    static FORCEINLINE constexpr T RadiansToDegrees(T Value) requires(TIsFloatingPoint<T>::Value)
    {
        return Value * static_cast<T>(180.0 / PI);
    }

public:
    
    // Other

    template<const uint64 NumBits>
    static FORCEINLINE constexpr uint64 MaxNum()
    {
        return (static_cast<uint64>(1) << NumBits) - 1;
    }
};

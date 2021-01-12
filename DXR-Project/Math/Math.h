#pragma once
#include "Float.h"

/*
* Math
*/

class Math
{
public:
	template <typename T>
	FORCEINLINE static T DivideByMultiple(T Value, UInt32 Alignment)
	{
		return static_cast<T>((Value + Alignment - 1) / Alignment);
	}

	template <typename T>
	FORCEINLINE static T AlignUp(T Value, T Alignment)
	{
		static_assert(std::is_integral<T>());

		const T mask = Alignment - 1;
		return ((Value + mask) & (~mask));
	}

	template <typename T>
	FORCEINLINE static T AlignDown(T Value, T Alignment)
	{
		static_assert(std::is_integral<T>());

		const T mask = Alignment - 1;
		return ((Value) & (~mask));
	}

	FORCEINLINE static Float Lerp(Float a, Float b, Float f)
	{
		return (-f * b) + ((a * f) + b);
	}

	template <typename T>
	FORCEINLINE static T Min(T a, T b)
	{
		return a <= b ? a : b;
	}

	template <typename T>
	FORCEINLINE static T Max(T a, T b)
	{
		return a >= b ? a : b;
	}
};
#pragma once
#include "Float.h"

/*
* Math
*/

class Math
{
public:
	template <typename T>
	FORCEINLINE static T DivideByMultiple(T Value, uint32 Alignment)
	{
		return static_cast<T>((Value + Alignment - 1) / Alignment);
	}

	template <typename T>
	FORCEINLINE static T AlignUp(T value, T alignment)
	{
		static_assert(std::is_integral<T>());

		const T mask = alignment - 1;
		return ((value + mask) & (~mask));
	}

	template <typename T>
	FORCEINLINE static T AlignDown(T value, T alignment)
	{
		static_assert(std::is_integral<T>());

		const T mask = alignment - 1;
		return ((value) & (~mask));
	}

	FORCEINLINE static float Lerp(float A, float B, float F)
	{
		return (-F * B) + ((A * F) + B);
	}
};
#pragma once
#include "Types.h"

/*
* Math Helpers
*/

template <typename T>
inline T DivideByMultiple(T Value, Uint32 Alignment)
{
	return static_cast<T>((Value + Alignment - 1) / Alignment);
}

template <typename T>
inline T AlignUp(T value, T alignment)
{
	static_assert(std::is_integral<T>());

	const T mask = alignment - 1;
	return ((value + mask) & (~mask));
}

template <typename T>
inline T AlignDown(T value, T alignment)
{
	static_assert(std::is_integral<T>());

	const T mask = alignment - 1;
	return ((value) & (~mask));
}
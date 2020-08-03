#pragma once
#include "Types.h"

template <typename T>
inline T DivideByMultiple(T Value, Uint32 Alignment)
{
	return static_cast<T>((Value + Alignment - 1) / Alignment);
}
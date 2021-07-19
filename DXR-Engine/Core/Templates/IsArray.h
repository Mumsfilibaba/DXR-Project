#pragma once
#include "Core/Types.h"

/* Check if type is an array type */
template<typename T>
struct TIsArray
{
    static constexpr bool Value = false;
};

template<typename T>
struct TIsArray<T[]>
{
    static constexpr bool Value = true;
};

template<typename T, int32 N>
struct TIsArray<T[N]>
{
    static constexpr bool Value = true;
};
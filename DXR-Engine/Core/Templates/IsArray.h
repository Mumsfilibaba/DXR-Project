#pragma once
#include "Core/Types.h"

template<typename T>
struct _TIsArray
{
    static constexpr bool Value = false;
};

template<typename T>
struct _TIsArray<T[]>
{
    static constexpr bool Value = true;
};

template<typename T, int32 N>
struct _TIsArray<T[N]>
{
    static constexpr bool Value = true;
};

/* Check if type is an array type */
template<typename T>
inline constexpr bool IsArray = _TIsArray<T>::Value;
#pragma once
"Core/Types.h"

template<typename T>
struct _TRemoveExtent
{
    using Type = T;
};

template<typename T>
struct _TRemoveExtent<T[]>
{
    using Type = T;
};

template<typename T, uint32 SIZE>
struct _TRemoveExtent<T[SIZE]>
{
    using Type = T;
};

/* Removes array type */
template<typename T>
using TRemoveExtent = typename _TRemoveExtent<T>::Type;
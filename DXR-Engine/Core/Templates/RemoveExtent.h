#pragma once
#include "Core/Types.h"

template<typename T>
struct _TRemoveExtent
{
    typedef T Type;
};

template<typename T>
struct _TRemoveExtent<T[]>
{
    typedef T Type;
};

template<typename T, uint32 SIZE>
struct _TRemoveExtent<T[SIZE]>
{
    typedef T Type;
};

/* Removes array type */
template<typename T>
using TRemoveExtent = typename _TRemoveExtent<T>::Type;

// TODO: TRemoveAllExtents
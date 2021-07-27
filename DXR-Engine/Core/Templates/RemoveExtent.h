#pragma once
#include "Core/Types.h"

/* Removes array type */
template<typename T>
struct TRemoveExtent
{
    typedef T Type;
};

template<typename T>
struct TRemoveExtent<T[]>
{
    typedef T Type;
};

template<typename T, uint32 SIZE>
struct TRemoveExtent<T[SIZE]>
{
    typedef T Type;
};

// TODO: TRemoveAllExtents
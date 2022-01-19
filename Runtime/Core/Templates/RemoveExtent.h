#pragma once
#include "Core/CoreTypes.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Removes array type

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

template<typename T, const int32 N>
struct TRemoveExtent<T[N]>
{
    typedef T Type;
};

// TODO: TRemoveAllExtents

#pragma once
#include "Core/CoreTypes.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsArray

template<typename T>
struct TIsArray
{
    enum { Value = false };
};

template<typename T>
struct TIsArray<T[]>
{
    enum { Value = true };
};

template<typename T, const int32 N>
struct TIsArray<T[N]>
{
    enum { Value = true };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsBoundedArray

template<typename T>
struct TIsBoundedArray
{
    enum { Value = false };
};

template<typename T, const int32 N>
struct TIsBoundedArray<T[N]>
{
    enum { Value = true };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsUnboundedArray

template<typename T>
struct TIsUnboundedArray
{
    enum { Value = false };
};

template<typename T>
struct TIsUnboundedArray<T[]>
{
    enum { Value = true };
};

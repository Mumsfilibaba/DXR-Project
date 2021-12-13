#pragma once
#include "Not.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Checks if two types are the same

template<typename T, typename U>
struct TIsSame
{
    enum
    {
        Value = false
    };
};

template<typename T>
struct TIsSame<T, T>
{
    enum
    {
        Value = true
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Checks if two types are not the same

template<typename T, typename U>
struct TIsNotSame
{
    enum
    {
        Value = TNot<TIsSame<T, U>>::Value
    };
};
#pragma once
#include "Not.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsSame

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
// TIsNotSame

template<typename T, typename U>
struct TIsNotSame
{
    enum
    {
        Value = TNot<TIsSame<T, U>>::Value
    };
};
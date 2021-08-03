#pragma once
#include "IsSame.h"
#include "RemoveCV.h"
#include "Or.h"

/* Determine if type is a floating point type */
template<typename T>
struct TIsFloatingPoint
{
    enum
    {
        Value = (TOr<
            TIsSame<float, typename TRemoveCV<T>::Type>,
            TIsSame<double, typename TRemoveCV<T>::Type>,
            TIsSame<long double, typename TRemoveCV<T>::Type>>::Value)
    };
};

template <typename T>
struct TIsFloatingPoint<const T>
{
    enum
    {
        Value = TIsFloatingPoint<T>::Value
    };
};

template <typename T>
struct TIsFloatingPoint<volatile T>
{
    enum
    {
        Value = TIsFloatingPoint<T>::Value
    };
};

template <typename T>
struct TIsFloatingPoint<const volatile T>
{
    enum
    {
        Value = TIsFloatingPoint<T>::Value
    };
};
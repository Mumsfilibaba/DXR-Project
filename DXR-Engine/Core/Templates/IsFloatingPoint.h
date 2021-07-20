#pragma once
#include "IsSame.h"
#include "RemoveCV.h"
#include "Or.h"

/* Determine if type is a floating point type */
template<typename T>
struct TIsFloatingPoint
{
    static constexpr bool Value = TOr<
        TIsSame<float, typename TRemoveCV<T>>::Type,
        TIsSame<double, typename TRemoveCV<T>>::Type,
        TIsSame<long double, typename TRemoveCV<T>>::Type >> ::Value;
};

template <typename T>
struct TIsFloatingPoint<const T>
{
    static constexpr bool Value = TIsFloatingPoint<T>::Value;
};

template <typename T>
struct TIsFloatingPoint<volatile T>
{
    static constexpr bool Value = TIsFloatingPoint<T>::Value;
};

template <typename T>
struct TIsFloatingPoint<const volatile T>
{
    static constexpr bool Value = TIsFloatingPoint<T>::Value;
};
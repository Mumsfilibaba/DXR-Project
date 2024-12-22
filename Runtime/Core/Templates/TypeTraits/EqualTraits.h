#pragma once
#include "Core/Templates/TypeTraits/BooleanTraits.h"

template<typename T, typename U>
struct TIsSame : TFalseType { };

template<typename T>
struct TIsSame<T, T> : TTrueType { };

template<typename T, typename U>
struct TIsNotSame
{
    static constexpr bool Value = TNot<TIsSame<T, U>>::Value;
};
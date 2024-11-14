#pragma once
#include "Core/Templates/TypeTraits/BooleanTraits.h"

template<typename T>
struct TIsLValueReference : TFalseType { };

template<typename T>
struct TIsLValueReference<T&> : TTrueType { };

template<typename T>
struct TIsRValueReference : TFalseType { };

template<typename T>
struct TIsRValueReference<T&&> : TTrueType { };

template<typename T>
struct TIsReference
{
    inline static constexpr bool Value = TOr<TIsLValueReference<T>, TIsRValueReference<T>>::Value;
};
#pragma once
#include "Core/Templates/TypeTraits.h"

template<typename T>
constexpr typename TUnderlyingType<T>::Type UnderlyingTypeValue(T Value) requires(TIsEnum<T>::Value)
{
    return static_cast<typename TUnderlyingType<T>::Type>(Value);
}
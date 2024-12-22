#pragma once
#include "Core/Templates/TypeTraits.h"

// Move an object by converting it into a r-value
template<typename T>
constexpr typename TRemoveReference<T>::Type&& Move(T&& Value) noexcept
{
    return static_cast<typename TRemoveReference<T>::Type&&>(Value);
}

// Forward an object by converting it into a r-value from an l-value
template<typename T>
constexpr T&& Forward(typename TRemoveReference<T>::Type& Value) noexcept
{
    return static_cast<T&&>(Value);
}

// Forward an object by converting it into a r-value from an r-value
template<typename T>
constexpr T&& Forward(typename TRemoveReference<T>::Type&& Value) noexcept
{
    return static_cast<T&&>(Value);
}
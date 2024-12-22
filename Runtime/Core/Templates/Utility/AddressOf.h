#pragma once
#include "Core/Templates/TypeTraits.h"

template<typename T>
inline T* AddressOf(T& Object) noexcept requires(TIsObject<T>::Value)
{
    return reinterpret_cast<T*>(&const_cast<char&>(reinterpret_cast<const volatile char&>(Object)));
}

template<typename T>
inline T* AddressOf(T& Object) noexcept requires(TNot<TIsObject<T>>::Value)
{
    return &Object;
}
#pragma once
#include "TypeTraits.h"

template<typename T>
inline T* AddressOf(T& Object) noexcept requires(TIsObject<T>::Value)
{
    return reinterpret_cast<T*>(&const_cast<CHAR&>(reinterpret_cast<const volatile CHAR&>(Object)));
}

template<typename T>
inline T* AddressOf(T& Object) noexcept requires(TNot<TIsObject<T>>::Value)
{
    return &Object;
}
#pragma once
#include "Core/CoreDefines.h"
#include "EnableIf.h"
#include "IsObject.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// AddressOf

template<typename T>
FORCEINLINE typename TEnableIf<TIsObject<T>::Value, T*>::Type AddressOf(T& Object) noexcept
{
    return reinterpret_cast<T*>(&const_cast<char&>(reinterpret_cast<const volatile char&>(Object)));
}

template<typename T>
FORCEINLINE typename TEnableIf<!TIsObject<T>::Value, T*>::Type AddressOf(T& Object) noexcept
{
    return &Object;
}

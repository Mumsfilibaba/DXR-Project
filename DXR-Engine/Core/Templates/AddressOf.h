#pragma once
#include "CoreDefines.h"
#include "EnableIf.h"
#include "IsObject.h"

/* Returns the address of an object */
template<typename T>
FORCEINLINE TEnableIf<TIsObject<T>::Value, T*>::Type  AddressOf( T& Object ) noexcept
{ 
    return reinterpret_cast<T*>(&const_cast<char&>(reinterpret_cast<const volatile char&>(Object)));
}
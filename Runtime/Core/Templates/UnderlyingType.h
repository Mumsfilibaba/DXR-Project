#pragma once
#include "Identity.h"
#include "IsPointer.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TUnderlyingType

template<typename T>
struct TUnderlyingType
{
    typedef typename TIdentity<__underlying_type(T)>::Type Type;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ToUnderlying

template<typename EnumType>
CONSTEXPR typename TUnderlyingType<EnumType>::Type ToUnderlying(EnumType Value)
{
    return static_cast<typename TUnderlyingType<EnumType>::Type>(Value);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ToInteger

template<typename T>
CONSTEXPR typename TEnableIf<TIsPointer<T>::Value, uintptr>::Type ToInteger(T Pointer)
{
    return reinterpret_cast<uintptr>(Pointer);
}
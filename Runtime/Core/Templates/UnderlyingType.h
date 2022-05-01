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
constexpr typename TUnderlyingType<EnumType>::Type ToUnderlying(EnumType Value)
{
    return static_cast<typename TUnderlyingType<EnumType>::Type>(Value);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ToInteger

template<typename T>
constexpr typename TEnableIf<TIsPointer<T>::Value, uint64>::Type ToInteger(T Pointer)
{
    return reinterpret_cast<uint64>(Pointer);
}
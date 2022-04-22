#pragma once
#include "Identity.h"
#include "IsEnum.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Retrieve the underlying type

template<typename T>
struct TUnderlyingType
{
    typedef TIdentity<__underlying_type(T)>::Type Type;
};

template<typename EnumType>
inline typename TUnderlyingType<EnumType>::Type ToUnderlying(EnumType Value)
{
    return static_cast<typename TUnderlyingType<EnumType>::Type>(Value);
}

template<typename T>
inline typename TEnableIf<TIsPointer<T>::Value, uint64>::Type ToInteger(T Pointer)
{
    return reinterpret_cast<uint64>(Pointer);
}
#pragma once
#include "RemoveReference.h"
#include "EnableIf.h"
#include "IsConst.h"

/* Move an object by converting it into a rvalue */
template<typename T>
constexpr typename TRemoveReference<T>::Type&& Move( T&& Arg ) noexcept
{
    return static_cast<typename TRemoveReference<T>::Type&&>(Arg);
}

/* Forward an object by converting it into a rvalue from an lvalue */
template<typename T>
constexpr T&& Forward( typename TRemoveReference<T>::Type& Arg ) noexcept
{
    return static_cast<T&&>(Arg);
}

/* Forward an object by converting it into a rvalue from an rvalue */
template<typename T>
constexpr T&& Forward( typename TRemoveReference<T>::Type&& Arg ) noexcept
{
    return static_cast<T&&>(Arg);
}

template<typename T>
FORCEINLINE typename TEnableIf<!TIsConst<T>::Value>::Type Swap( T& LHS, T& RHS ) noexcept
{
    T TempElement = Move( LHS );
    LHS = Move( RHS );
    RHS = Move( TempElement );
}

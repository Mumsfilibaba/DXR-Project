#pragma once
#include "RemoveReference.h"
#include "EnableIf.h"
#include "IsConst.h"
#include "Not.h"

/* Move an object by converting it into a rvalue */
template<typename T>
constexpr typename TRemoveReference<T>::Type&& Move( T&& Value ) noexcept
{
    return static_cast<typename TRemoveReference<T>::Type&&>(Value);
}

/* Forward an object by converting it into a rvalue from an lvalue */
template<typename T>
constexpr T&& Forward( typename TRemoveReference<T>::Type& Value ) noexcept
{
    return static_cast<T&&>(Value);
}

/* Forward an object by converting it into a rvalue from an rvalue */
template<typename T>
constexpr T&& Forward( typename TRemoveReference<T>::Type&& Value ) noexcept
{
    return static_cast<T&&>(Value);
}

template<typename T>
FORCEINLINE typename TEnableIf<TNot<TIsConst<T>>::Value>::Type Swap( T& LHS, T& RHS ) noexcept
{
    T TempElement = Move( LHS );
    LHS = Move( RHS );
    RHS = Move( TempElement );
}

/* Helps expand variadic packs */
template<typename... Packs>
inline void ExpandPacks( Packs&&... )
{
}
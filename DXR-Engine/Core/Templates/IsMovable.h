#pragma once
#include "AddReference.h"
#include "IsConstructable.h"
#include "IsAssignable.h"

/* Checks weather the type can be constructed with a move constructor */
template<typename T>
struct TIsMoveConstructable
{
    static constexpr bool Value = TIsConstructable<T, typename TAddRightReference<T>::Type>::Value;
};

/* Checks weather the type can be constructed with a move assignment operator */
template<typename T>
struct TIsMoveAssignable
{
    static constexpr bool Value = TIsAssignable<T, typename TAddRightReference<T>::Type>::Value;
};
#pragma once
#include "AddReference.h"
#include "IsConstructable.h"
#include "IsAssignable.h"

/* Checks weather the type can be constructed with a copy constructor */
template<typename T>
struct TIsCopyConstructable
{
    static constexpr bool Value = TIsConstructable<T, typename TAddLeftReference<const T>::Type>::Value;
};

/* Checks weather the type can be constructed with a copy assignment operator */
template<typename T>
struct TIsCopyAssignable
{
    static constexpr bool Value = TIsAssignable<T, typename TAddLeftReference<const T>::Type>::Value;
};
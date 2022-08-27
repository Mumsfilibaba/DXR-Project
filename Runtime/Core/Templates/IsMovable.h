#pragma once
#include "AddReference.h"
#include "IsConstructible.h"
#include "IsAssignable.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsMoveConstructable

template<typename T>
struct TIsMoveConstructable
{
    enum { Value = TIsConstructible<T, typename TAddRValueReference<T>::Type>::Value };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsMoveAssignable

template<typename T>
struct TIsMoveAssignable
{
    enum { Value = TIsAssignable<T, typename TAddRValueReference<T>::Type>::Value };
};